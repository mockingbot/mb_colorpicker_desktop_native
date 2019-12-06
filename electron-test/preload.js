const { execFile } = require('child_process')
const { join } = require('path')

// BrowserWindow.preload: Specifies a script that will be loaded before other scripts run in the page.
// check `webPreferences.preload` in: https://electronjs.org/docs/api/browser-window#new-browserwindowoptions

const IS_DARWIN_PERMISSION_CHECK_REQUIRED = process.platform === 'darwin'

const PATH_COLORPICKER_BINARY = join(__dirname, '../_dst/', process.platform === 'darwin' ? 'macOS/ColorPicker'
  : process.platform === 'win32' ? `Windows/${process.arch}/ColorPicker.exe`
    : ''
)

const SYSTEM_INFO_STRING = [
  `OS: ${process.platform} (${process.arch})`,
  `IS_DARWIN_PERMISSION_CHECK_REQUIRED: ${IS_DARWIN_PERMISSION_CHECK_REQUIRED}`,
  `PATH_COLORPICKER_BINARY: ${PATH_COLORPICKER_BINARY}`
].join('\n')

const EXEC_COLORPICKER_ASYNC = async (...args) => new Promise((resolve, reject) => execFile(
  PATH_COLORPICKER_BINARY, // file
  args,
  (error, stdout, stderr) => {
    if (error) {
      console.error(`[Error|EXEC_COLORPICKER_ASYNC]`, error)
      return reject(error)
    }
    stderr && console.warn(`[stderr|EXEC_COLORPICKER_ASYNC]`, stderr)
    stdout && console.log(`[stdout|EXEC_COLORPICKER_ASYNC]`, stdout)
    resolve({ stdout, stderr })
  }
))

// TODO: NOTE: above JS context: current + Browser + Electron
process.once('loaded', () => { // TODO: NOTE: below JS context: current + Browser
  window.PRELOAD = { // expose to renderer process (Browser)
    SYSTEM_INFO_STRING,
    IS_DARWIN_PERMISSION_CHECK_REQUIRED,
    EXEC_COLORPICKER_ASYNC
  }
})
