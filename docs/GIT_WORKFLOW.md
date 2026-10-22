# GitHub 与 Codex 协作流程

本轮不 commit、不 push、不建 PR、不改远程设置；下文是团队开始编码后的执行规范。主分支 `main` 保持稳定可演示，五人独立工作目录、独立 Codex、独立任务分支。

## 分支与提交

一个可验收任务/子任务一个短期分支，例 `feature/b/DEV-003-crop-state`、`fix/c/DEV-002-map-bounds`、`docs/a/team-plan`。不用长期五条模块分支积累数周差异。任务很大时先合兼容接口，再多个可编译小 PR；最终父任务仍统一验收。

开工先 `git status --short`，不得覆盖他人未提交改动；确认任务认领和接口基线。干净 main 上 `git fetch origin`、`git switch main`、`git pull --ff-only`，然后 `git switch -c <task-branch>`。已发布多人使用的分支用 `git merge origin/main` 同步；个人未发布分支可 rebase。主分支禁止强推；共享分支不重写历史。执行 push 前确认该任务已获用户/团队实际授权，文档建议不是本轮上传授权。

Commit 使用 `type(scope): 内容`，如 `feat(inventory): reject harvest when backpack is full`、`fix(map): reject negative world coordinates`、`docs(team): define file ownership`。正文说明触发条件、变更和验证；不将编译输出、本机路径、个人存档混入。

## PR 模板正文（未来由 A 写入 .github/pull_request_template.md）

```markdown
## 任务与行为
- DEV 编号、模块/负责人：
- 本次解决的问题与可观察结果：
- 基线 main SHA / 接口 IC 版本：

## 文件归属与契约
- 修改文件和主责；跨域认领/审查人：
- 公共接口/资源/构建/存档/配置变化（无则写无）：
- 兼容与迁移顺序：

## 验证证据
- 应用 qmake 构建命令、Kit、结果：
- 测试命令/数量/结果与失败项：
- GUI 操作记录/截图（涉及界面时）：
- 尚未执行与已知限制：

## 合并与回退
- 前置 PR、必须审查的消费者：
- 回退方法及存档/配置兼容影响：
- 对应任务的 DoD 检查：
```

## Review 和集成

普通模块 PR 至少一名非作者审查；公共接口须 A 与直接消费者确认，A 自己写的接口不能自审替代消费者。业务/数量断言由领域负责人审，UI 由 C 审，跨模块事务由 A/B/D/E 按涉及范围审。A 统一合并（推荐 squash 保留任务可回退单位），成员离线可明确指定一名代理，不默认为任何人都能覆盖 main。

main 保护建议：禁止强推、至少一名非作者批准、Windows 应用+Qt Test 检查通过；仓库权限/套餐是否支持待 A 确认，本轮未启用。真实账号未提供前不写假的 CODEOWNERS。文件归属先以 FILE_OWNERSHIP 生效，账号到位后转为机器可识别规则，且测试文件例外必须排在广泛目录规则之后。

接口变更顺序：契约 diff→相关负责人评审→A 合兼容公共头→记录 SHA→消费者同步 main→分别提交实现→A/E 集成测试→移除过渡入口。禁止接口 PR 与消费者 PR 同时以不同签名各自通过本地假服务测试后直接合并。

合并前 fetch 最新 main，重跑受影响构建与测试；同时修改公共类型/状态/构建时，必须跑全套已建立目标。失败先停合并，修复或回退；不要在失败 main 上继续堆业务。重复检查仅在新修改/新基线/失败需要时执行。

## 高频共享文件流程

| 文件 | 谁实际编辑 | 其他成员提供什么 | 合并检查 |
| --- | --- | --- | --- |
| CHECK.pro、tests/tests.pro | A | 新源文件/模块 .pri、需要的 Qt 模块、测试目标命令 | 既有目标不漏编、新文件不重复、测试 exe 都执行 |
| 公共 core 头/接口文档 | A | 字段/签名 diff、消费者和存档影响、最小测试 | 消费方确认同一接口版本 |
| MainWindow 组装 | C | A 的 runtime 创建/销毁 API；B/D/E 提供事件/查询 | 生命周期、只连一次信号、无资产副本 |
| character.qrc/资源 | C | 实体路径、稳定 alias、用途、切图坐标/来源 | rcc 和运行 URL、旧素材不丢 |
| 公共测试 fixture | A | 各域最小需求，不含私有业务替代实现 | ID/初态统一、没有第二套库存模型 |

认领与锁定记录写在任务 Issue/PR，而不是口头告知另一个 Codex。若两个任务需要同一文件，先拆接口、排序小 PR 或授权一个成员统一编辑。Git 自动合并成功也不能证明业务无冲突。

## 冲突与回退

遇到冲突，保存当前工作，核对双方任务和最新所有权，由文件主责解释语义；A 协调合并顺序。不可整文件选 ours/theirs、重新生成整份 README 或恢复旧公共头。涉及签名变化先解决契约，再合实现、构建、测试和文档。

导致 main 集成失败时由 A 用新的 revert 提交回退问题合并，修复分支重新验证后再合；不 reset/force-push main。含存档版本变化的 PR 先确认旧程序能否读取新档；不能读则保留存档备份，优先兼容修复或明确阻止旧版本覆盖，不以删用户存档解决。

## 已追踪本机文件与安全清理建议

Git 当前已经追踪 `CHECK.pro.user`、`map.tiled-session`、`map7.tiled-session`、`build/` 下 Qt 6.8.2 的缓存/对象/生成物，以及 `wav/Audacity_Installer_via_MuseHub.exe`。详见 DIRECTORY_STRUCTURE 的真实清单。.gitignore 已忽略 *.pro.user、*.tiled-session、build/、build_*/、*.exe，但忽略规则不会取消已有追踪。`build-readme-check/` 使用连字符，不匹配 build_*/，当前仍是未追踪目录。

本轮全部保留。未来单独清理 PR：A 先列出 git ls-files 命中清单，C 查明资源是否引用安装程序或输出；保存本机配置/所需文件，再仅从索引取消指定缓存/配置/构建产物追踪，完善忽略规则。取消追踪不会自动清理历史 Git 体积，不为此重写远程历史；绝不把 wav/ 或全部 *.exe 一概当作可删素材。Tiled 工程 `.tiled-project` 与会话文件不同，保留项目配置。

每个 Codex 在提交前比对 `git diff --name-only`、所有权、契约版本和 `git diff --check`；只显式 stage 本任务文件，不 `git add .` 把本地缓存混进去。
