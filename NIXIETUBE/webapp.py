#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Clock Web App — 时间校准 + 实时显示

一套代码，两处运行：
  * PC 平台    ：自动使用系统时钟（没有 /dev/ds3231），便于本地开发调试
  * 嵌入式板卡 ：自动使用 DS3231 RTC（/dev/ds3231），读写硬件时钟

DS3231 字符设备接口（由 ds3231.ko 提供）：
  * read(fd, 3)  返回 3 字节 [秒, 分, 时]，阻塞式，随 SQW 1Hz 中断每秒返回一次
  * write(fd, 3) 写入 3 字节 [时, 分, 秒]

HTTP 接口：
  GET  /api/time   -> 当前时间 {hour, minute, second, source}
  POST /api/time   -> 校准时间 {hour, minute, second}
  GET  /api/status -> 后端信息 {source, has_rtc, ...}
"""

import argparse
import ctypes
import datetime
import json
import os
import sys
import threading
import time

from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

DS3231_DEV = "/dev/ds3231"


# ---------------------------------------------------------------- 系统时钟同步

class _Timespec(ctypes.Structure):
    _fields_ = [("tv_sec", ctypes.c_longlong), ("tv_nsec", ctypes.c_longlong)]


_CLOCK_REALTIME = 0


def _load_libc():
    """加载 libc（glibc 用 libc.so.6，musl 用 libc.so，最后回退主程序句柄）。"""
    for name in (None, "libc.so.6", "libc.so"):
        try:
            return ctypes.CDLL(name, use_errno=True)
        except OSError:
            continue
    return None


def _clock_settime(epoch_seconds):
    """设置 Linux 系统时钟（CLOCK_REALTIME），成功后 `date` 立即更新。

    传入 epoch 秒；调用方负责把墙钟时间转成 epoch。这里不处理时区，
    调用方用 time.mktime（按设备本地时区解释）保证 `date` 显示的墙钟时间
    与输入一致。
    """
    try:
        libc = _load_libc()
        if libc is None:
            return False
        libc.clock_settime.argtypes = [ctypes.c_int, ctypes.POINTER(_Timespec)]
        libc.clock_settime.restype = ctypes.c_int
        ts = _Timespec(int(epoch_seconds), 0)
        return libc.clock_settime(_CLOCK_REALTIME, ctypes.byref(ts)) == 0
    except Exception as e:
        print("clock_settime failed: %s" % e, file=sys.stderr)
        return False


# ---------------------------------------------------------------- 时间后端

class TimeBackend:
    """时间源抽象：PC 用系统时钟，板卡用 DS3231。"""
    name = "unknown"

    def get_time(self):
        raise NotImplementedError

    def set_time(self, dt):
        raise NotImplementedError

    def info(self):
        return {"source": self.name}


class SystemClockBackend(TimeBackend):
    """无 RTC 模式：直接读写系统时钟。

    * 板卡未接 DS3231（或 /dev/ds3231 不存在）时用它：set_time 真正设置系统时钟，
      这样即使没有硬件 RTC，`date` 也能被手机校准。
    * PC 开发调试时用 --dev 让它变成 no-op，避免改动宿主机真实时间。
    """
    name = "system"

    def __init__(self, dev=False):
        self._dev = dev

    def get_time(self):
        now = datetime.datetime.now()
        return {
            "hour": now.hour,
            "minute": now.minute,
            "second": now.second,
            "date": now.strftime("%Y-%m-%d"),
            "weekday": now.strftime("%A"),
        }

    def set_time(self, dt):
        if self._dev:
            print("[system] set_time -> %s (dev no-op)" %
                  dt.strftime("%Y-%m-%d %H:%M:%S"), file=sys.stderr)
            return True
        ok = _clock_settime(time.mktime(dt.timetuple()))
        if not ok:
            print("[system] set system clock failed", file=sys.stderr)
        return ok


class Ds3231Backend(TimeBackend):
    """板卡模式：读写 /dev/ds3231。

    read() 是阻塞式且每秒只返回一次（随 SQW 中断），
    所以单独起一个后台线程持续读取并缓存最新时间，HTTP 请求只读缓存。
    """
    name = "ds3231"

    def __init__(self):
        self._lock = threading.Lock()
        self._cache = None
        self._write_fd = None
        self._stop = threading.Event()

        try:
            self._write_fd = os.open(DS3231_DEV, os.O_RDWR | os.O_NONBLOCK)
        except OSError as e:
            print("ds3231: open for write failed: %s" % e, file=sys.stderr)

        self._reader = threading.Thread(target=self._read_loop, daemon=True)
        self._reader.start()

    def _read_loop(self):
        fd = None
        try:
            fd = os.open(DS3231_DEV, os.O_RDONLY)
        except OSError as e:
            print("ds3231: open for read failed: %s" % e, file=sys.stderr)
            return

        first = True
        while not self._stop.is_set():
            try:
                b = os.read(fd, 3)
                if len(b) == 3:
                    sec, mnt, hour = b[0], b[1], b[2]
                    with self._lock:
                        self._cache = {
                            "hour": hour,
                            "minute": mnt,
                            "second": sec,
                            "date": "",
                            "weekday": "",
                        }
                    if first:
                        first = False
                        self._restore_system_clock(hour, mnt, sec)
            except OSError:
                time.sleep(0.2)
        os.close(fd)

    def _restore_system_clock(self, hour, minute, second):
        """开机时用 RTC 恢复系统时钟的时分秒。

        DS3231 驱动只暴露时分秒，没有日期，因此日期沿用系统当前值
        （完整日期需等手机校时下发）。仅恢复一次，避免和后续校准打架。
        """
        now = datetime.datetime.now()
        dt = datetime.datetime(now.year, now.month, now.day,
                                int(hour), int(minute), int(second))
        _clock_settime(time.mktime(dt.timetuple()))

    def get_time(self):
        with self._lock:
            return self._cache

    def set_time(self, dt):
        if self._write_fd is None:
            return False

        # 1) 写 DS3231 的时分秒
        rtc_ok = False
        buf = bytes([int(dt.hour), int(dt.minute), int(dt.second)])
        try:
            n = os.write(self._write_fd, buf)
            rtc_ok = (n == 3)
        except OSError as e:
            print("ds3231: write failed: %s" % e, file=sys.stderr)

        # 2) 同步系统时钟（完整年月日时分秒），确保 date 与 RTC 一起校准
        sys_ok = _clock_settime(time.mktime(dt.timetuple()))
        if not sys_ok:
            print("ds3231: set system clock failed", file=sys.stderr)

        return rtc_ok and sys_ok

    def info(self):
        return {
            "source": self.name,
            "has_rtc": self._write_fd is not None,
            "dev": DS3231_DEV,
        }


def detect_backend(dev=False):
    if os.path.exists(DS3231_DEV):
        return Ds3231Backend()
    return SystemClockBackend(dev=dev)


# ---------------------------------------------------------------- HTTP 服务

class ClockHandler(BaseHTTPRequestHandler):
    backend = None  # 由 main() 注入
    www_dir = None  # 由 main() 注入

    # --- 工具方法 ---
    def _send_json(self, obj, code=200):
        body = json.dumps(obj).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    def _send_file(self, path):
        if not os.path.isfile(path):
            self._send_json({"error": "not found"}, 404)
            return
        with open(path, "rb") as f:
            body = f.read()
        ctype = "text/html"
        if path.endswith(".js"):
            ctype = "application/javascript"
        elif path.endswith(".css"):
            ctype = "text/css"
        elif path.endswith(".svg"):
            ctype = "image/svg+xml"
        self.send_response(200)
        self.send_header("Content-Type", "%s; charset=utf-8" % ctype)
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Cache-Control", "no-store")
        self.end_headers()
        self.wfile.write(body)

    # --- 路由 ---
    def do_GET(self):
        if self.path == "/" or self.path == "/index.html":
            return self._send_file(os.path.join(self.www_dir, "index.html"))
        if self.path == "/app.js":
            return self._send_file(os.path.join(self.www_dir, "app.js"))
        if self.path == "/style.css":
            return self._send_file(os.path.join(self.www_dir, "style.css"))
        if self.path == "/api/time":
            t = self.backend.get_time()
            if t is None:
                return self._send_json({"pending": True, "source": self.backend.name})
            t = dict(t)
            t["source"] = self.backend.name
            t["pending"] = False
            return self._send_json(t)
        if self.path == "/api/status":
            return self._send_json(self.backend.info())
        return self._send_json({"error": "not found"}, 404)

    def do_POST(self):
        if self.path != "/api/time":
            return self._send_json({"error": "not found"}, 404)

        try:
            length = int(self.headers.get("Content-Length", 0))
            data = json.loads(self.rfile.read(length).decode("utf-8"))
            year = int(data["year"])
            month = int(data["month"])
            day = int(data["day"])
            hour = int(data["hour"])
            minute = int(data["minute"])
            second = int(data["second"])
            dt = datetime.datetime(year, month, day, hour, minute, second)
        except (ValueError, KeyError, TypeError, json.JSONDecodeError):
            return self._send_json({"ok": False, "error": "bad payload"}, 400)

        if not (0 <= hour <= 23 and 0 <= minute <= 59 and 0 <= second <= 59):
            return self._send_json({"ok": False, "error": "invalid time"}, 400)

        ok = self.backend.set_time(dt)
        return self._send_json({
            "ok": ok,
            "set": dt.strftime("%Y-%m-%d %H:%M:%S"),
            "source": self.backend.name,
        })

    def log_message(self, fmt, *args):
        # 精简日志，避免刷屏
        sys.stderr.write("%s - %s\n" % (self.address_string(), fmt % args))


def find_www(opt):
    if opt:
        return opt
    candidates = [
        "/www",  # 嵌入式 overlay 落点
        os.path.join(os.path.dirname(os.path.abspath(__file__)), "www"),  # PC 同目录
    ]
    for c in candidates:
        if os.path.isdir(c):
            return c
    return None


def main():
    parser = argparse.ArgumentParser(description="Clock Web App")
    parser.add_argument("--port", type=int, default=80, help="监听端口 (PC 建议 8080)")
    parser.add_argument("--www", type=str, default=None, help="静态资源目录")
    parser.add_argument("--dev", action="store_true",
                        help="开发模式：system 后端不真正修改系统时钟")
    args = parser.parse_args()

    www = find_www(args.www)
    if not www:
        print("错误：找不到 www 目录，请用 --www 指定", file=sys.stderr)
        sys.exit(1)

    backend = detect_backend(dev=args.dev)
    print("时间源: %s%s" % (backend.name,
          " (dev no-op)" if args.dev and backend.name == "system" else ""),
          file=sys.stderr)
    print("静态目录: %s" % www, file=sys.stderr)

    ClockHandler.backend = backend
    ClockHandler.www_dir = www

    server = ThreadingHTTPServer(("0.0.0.0", args.port), ClockHandler)
    print("Web 服务已启动: http://0.0.0.0:%d" % args.port, file=sys.stderr)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
