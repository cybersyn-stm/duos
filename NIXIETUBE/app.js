/* Clock Web App 前端逻辑 */
(function () {
  'use strict';

  var els = {
    h1: document.getElementById('h1'),
    h2: document.getElementById('h2'),
    m1: document.getElementById('m1'),
    m2: document.getElementById('m2'),
    s1: document.getElementById('s1'),
    s2: document.getElementById('s2'),
    meta: document.getElementById('meta'),
    syncBtn: document.getElementById('syncBtn'),
  };

  function pad2(n) {
    n = parseInt(n, 10);
    if (isNaN(n)) n = 0;
    return n < 10 ? '0' + n : '' + n;
  }

  function render(h, m, s) {
    var hs = pad2(h), ms = pad2(m), ss = pad2(s);
    els.h1.textContent = hs[0];
    els.h2.textContent = hs[1];
    els.m1.textContent = ms[0];
    els.m2.textContent = ms[1];
    els.s1.textContent = ss[0];
    els.s2.textContent = ss[1];
  }

  function localNow() {
    var d = new Date();
    return {
      year: d.getFullYear(),
      month: d.getMonth() + 1,
      day: d.getDate(),
      hour: d.getHours(),
      minute: d.getMinutes(),
      second: d.getSeconds()
    };
  }

  // 用本机（手机/PC）时间校准设备时钟
  function syncTime() {
    var t = localNow();
    els.meta.textContent = '正在校时…';
    fetch('/api/time', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(t),
    })
      .then(function (r) { return r.json(); })
      .then(function (j) {
        if (j.ok) {
          els.meta.textContent = '已校准 · 数据源: ' + (j.source || '') + ' → ' + j.set;
        } else {
          els.meta.textContent = '校时失败: ' + (j.error || '未知错误');
        }
      })
      .catch(function (e) {
        els.meta.textContent = '校时失败: ' + e;
      });
  }

  // 每秒轮询设备时间，更新显示
  function poll() {
    fetch('/api/time')
      .then(function (r) { return r.json(); })
      .then(function (j) {
        if (j.pending) {
          els.meta.textContent = '等待 RTC 数据…';
          return;
        }
        render(j.hour, j.minute, j.second);
        els.meta.textContent = '数据源: ' + (j.source || '') + ' · ' + pad2(j.hour) + ':' + pad2(j.minute) + ':' + pad2(j.second);
      })
      .catch(function (e) {
        els.meta.textContent = '连接中断，重试中…';
      });
  }

  els.syncBtn.addEventListener('click', syncTime);

  // 打开页面即自动校时一次（方案 A：手机下发时间）
  syncTime();
  poll();
  setInterval(poll, 1000);
})();
