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
