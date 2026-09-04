#!/bin/bash

# 切换到 main 分支
git checkout main

# 添加所有更改
git add .

# 提交，信息为日期加参数
git commit -m "$(date '+%Y-%m-%d') $1"

# 推送到远程 main 分支
git push origin main
