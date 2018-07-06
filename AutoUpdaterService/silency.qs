function Controller()
{
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

Controller.prototype.ComponentSelectionPageCallback = function()
{
    var widget = gui.currentPageWidget(); // get the current wizard page
    if (widget != null) {
        gui.clickButton(buttons.NextButton);
    }
}

Controller.prototype.ReadyForInstallationPageCallback = function()
{
    var widget = gui.currentPageWidget(); // get the current wizard page
    if (widget != null) {
        gui.clickButton(buttons.CommitButton);
    }
}

Controller.prototype.PerformInstallationPageCallback = function()
{
    gui.hide();
    var widget = gui.currentPageWidget(); // get the current wizard page
    if (widget != null) {
        gui.clickButton(buttons.CommitButton);
    }
}

Controller.prototype.InstallationFinishedPageCallback = function()
{
    var widget = gui.currentPageWidget(); // get the current wizard page
    if (widget != null) {
        gui.clickButton(buttons.FinishButton);
    }
}
