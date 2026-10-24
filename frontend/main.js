const { app, BrowserWindow, screen } = require("electron");
const path = require("path");

function createWindow() {
  const { workArea } = screen.getPrimaryDisplay();

  const width = 380;
  const height = 560;

  const win = new BrowserWindow({
    width,
    height,
    x: workArea.x + workArea.width - width - 20,
    y: workArea.y + workArea.height - height - 20,
    alwaysOnTop: true,
    resizable: true,
    autoHideMenuBar: true,
    webPreferences: {
      nodeIntegration: false,
      contextIsolation: true
    }
  });

  win.loadFile(path.join(__dirname, "index.html"));
}

app.whenReady().then(createWindow);

app.on("window-all-closed", () => {
  app.quit();
});
