# third_party

## SenseDesign — Git 子模块（只读依赖）

| 路径 | 方式 |
|------|------|
| `SenseDesign/` | **git submodule** → https://github.com/qfotcom/SenseDesign.git |

UI / `SenseAppShell` **不在本仓库改**；升级用 `git submodule update` + 提交新指针。与 [SenseWorkflow](../SenseWorkflow) 使用同一套 shadcn QML 分支约定（`feature/shadcn-qml-refactor`）。

首次克隆本仓库后：

```powershell
.\tools\setup_submodules.ps1
# 或
git submodule update --init third_party/SenseDesign
```
