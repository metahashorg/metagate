function Controller()
{
do {
    if (!installer.isProcessRunning("MetaGate.exe"))
        break;
        var result = QMessageBox.question("quit.question", "Start Program", "Do you want to start the installed application?", QMessageBox.Retry | QMessageBox.Close);
    if( result == QMessageBox.Close) {
    installer.setValue("FinishedText", "<font color='red' size=3>The installer was quit.</font>");
            installer.setDefaultPageVisible(QInstaller.Introduction, false);
            installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
            installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
            installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
            installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
            installer.setDefaultPageVisible(QInstaller.PerformInstallation, false);
            installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
            //installer.setDefaultPageVisible(QInstaller.InstallationFinished, false);
            gui.clickButton(buttons.NextButton);
            return;
}

} while (true);
}

Controller.prototype.IntroductionPageCallback = function()
{
var widget = gui.currentPageWidget(); // get the current wizard page
if (widget != null) {
    var updaterRadioButton = widget.findChild("UpdaterRadioButton");
    updaterRadioButton.checked = true;
    gui.clickButton(buttons.NextButton);
    //widget.title = "New title."; // set the page title
    //widget.MessageLabel.setText("New Message."); // set the welcome text
}
}
