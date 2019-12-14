# mb_colorpicker_desktop_native

An color picker for Win and Mac with pure native platform API

------------
## 使用说明
从2.0版本起，取色器运行时会**同时从 stderr 和 stdout 输出信息**：

* stderr 仅有程序运行时相关的函数track，并**不包含**最终的数据信息：

    这一数据，仅需在程序出现崩溃、或者客户反映未能正常取色时，提交反馈给后台，用以辅助定位程序故障；
    一般情况下，无需处理。

* stdout 中**包含**最终了所需的数据信息：

    所有平台下，在不指定命令行参数（即默认`--mode=0`）时，程序以屏幕取色功能模式运行，
    stdout 数据为最终的取色结果输出，格式为`#XXXXXX\n`（井号+大写RGB值+换行符\n）；
    用户按下esc键后取消取色，此时 stdout 数据为空。

    其他命令行参数在不同平台之间各不相同，为此，相关内容请仔细阅读以下辅助说明。


### macOS平台
由于 macOS 在 10.15 版本后增加了屏幕捕获API的权限检测，
因此，若未经用户允许，直接运行取色器程序通常而言无法获得期望结果。
为了支持 macOS 的这一改动，取色器额外添加了两种功能模式：

* <strong>`屏幕捕获权限检测`功能，命令行参数`--mode=1`：</strong>

    这一模式下，stdout 为权限检测结果输出。
    若权限允许，完整输出信息为"`Screen Record Permission Granted: YES\n`"，
    反之为"`Screen Record Permission Granted: NO\n`"。
    用户可根据数据信息中是否包含字符串 YES，或者字符串 NO，确认权限状况。

    需要额外说明的是，在低于10.15的 macOS 中，即10.14、10.13等版本，由于无需进行相应权限的申请，
    此时 stdout 输出始终为"`Screen Record Permission Granted: YES\n`"。

    此外，运行这一功能模式并**不会导致权限选择窗口弹出**。

* <strong>`触发权限选择窗口弹出`功能，命令行参数`--mode=2`：</strong>

    为提升用户使用体验，除了上述权限检测功能外，
    完整的权限处理流程还需配合本功能用以触发macOS权限选择窗口的弹出。

    在这一功能的代码实现上，首先使用了系统内置 tccutil 命令清除相应程序的权限记录，
    随后是调用屏幕捕获API，如此一来，确保了权限选择窗口**一定可以弹出**。
    
    由于 tccutil 在清除权限记录时需要指定目标程序的 bundle-id，因此调用此功能时共需两个命令行参数。
    以墨刀桌面版为例，此程序 bundle-id 为 com.MockingBot.MockingBotMAC，
    因而，这一功能模式完整的命令行参数为: `ColorPicker --mode=2 --bundle-id=com.MockingBot.MockingBotMAC`。
    
    **特别需要注意的是**，所谓的`目标程序`是指取色器初始启动进程，例如：
    若在 macOS 系统自带 Terminal 程序下启动取色器调试，则此时目标程序为 Terminal ，需要指定的 bundle-id 为 Terminal 的 bundle-id，而非
     Electron 的 bundle-id。
    同理，若在 iTerm2 下启动取色器调试，则此时目标程序为 iTerm2。
    若取色器程序最终打包进桌面版的墨刀，即 MockingBot 程序，那么所谓的目标程序──在正常从启动面板上点击启动的话──就是MockingBot。
    
    这一模式下，stdout 的输出随 tccutil 执行结果的不同而不同。
    如果 tccutil 成功清除了指定程序的权限记录，
    那么完整的输出信息为"`Promote Screen Record Permission Grant Window Succeeded: YES\n`"，
    反之为"`Promote Screen Record Permission Grant Window Succeeded: NO\n`"。
    
    程序会在触发系统权限弹窗后退出，因此不能在本次返回权限结果，
    同时由于用户操作时长未知，建议在用户下次触发取色时进行`--mode=1`获得权限检测结果。
    
    建议参考以下代码结构：
    ```js
    const pickColorAsync = async () => {
      if (await MODE_1_CHECK_PERMISSION() === false) {
        await MODE_2_REQUEST_PERMISSION('com.bundle.id')
        return '' // bail and wait for next permission check
      }
      return MODE_0_PICK_COLOR()
    }
    ```

### Windows平台辅助说明
TODO：

------------
## structure
the file:
- source:
  * `src/` - source code for both darwin & win32 
  * `res/` - image resource
- output: `_dst/` - binary output
- test: `test/` - test Electron app, use binary from `_dst/`


## build
if you have `npm` installed:
```shell script
npm run build-win32
npm run build-darwin
```
or check `scripts` in [./package.json](./package.json) for command


## electron test
first setup electron test with: 
```shell script
npm run electron-test-install
```

then run:
```shell script
npm run electron-test-start
```
