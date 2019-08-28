function Controller() {
}

Controller.prototype.IntroductionPageCallback = function() {
	var widget = gui.currentPageWidget(); // get the current wizard page
    if (widget != null) {
    	if (installer.isOfflineOnly()) {
        	widget.MessageLabel.setText("Helojjfgjkg gfjkjkfg ffgjkfjkf fkfgjfgj fkfgjkjfgro gkkjgfkjgf fkgjgj kjgfjgfk g kffgjg kfgfkjjk kjfgjg");
        }
    }
}


Controller.prototype.TargetDirectoryPageCallback = function()
{
    if (systemInfo.productType === "osx") {
        var page = gui.pageWidgetByObjectName("TargetDirectoryPage");
    	page.TargetDirectoryLineEdit.setText(installer.value("HomeDir") + "/Applications/MetaGate");
    }
}

/*
Controller.prototype.FinishedPageCallback = function()
{
	console.log("jkfjkfjfjfgjkfgkjfgjfg")
	var widget = gui.currentPageWidget(); // get the current wizard page
    if (widget != null) {
    	widget.CommitButton.setText("jffgjkj");
    	widget.CommitButton.setEnabled(fasle);
    }
}
*/
Controller.prototype.FinishedPageCallback = function()
{
    var page = gui.pageWidgetByObjectName("FinishedPage");
    gui.clickButton(buttons.FinishButton);
}
