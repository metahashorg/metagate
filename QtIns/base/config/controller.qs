function Controller() {
}

Controller.prototype.IntroductionPageCallback = function() {
/*	var widget = gui.currentPageWidget(); // get the current wizard page
    if (widget != null) {
    	if (installer.isOfflineOnly()) {
        	widget.MessageLabel.setText("<font size='+1'><h2><font color='red'>Please note:</font></h2><p>MetaGate installer has been changed. It was made to improve the work of the app and add new features in the future. The old version of MetaGate is deleted automatically. You need to install a new one only. All wallets and settings won't be changed. Don't forget to have a copy of the private keys/QR-code of the wallet at a safe place like a USB flash drive.</p></font>");
        }
    }*/
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
