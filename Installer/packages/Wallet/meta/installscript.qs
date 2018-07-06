function Component()
{
    // default constructor
}

Component.prototype.createOperations = function()
{
    // call default implementation to actually install WalletMetahash.exe
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", "@TargetDir@/WalletMetahash.exe", "@StartMenuDir@/WalletMetahash.lnk",
            "workingDirectory=@TargetDir@", "iconPath=%SystemRoot%/system32/SHELL32.dll",
            "iconId=0", "description=Launch Metahash Wallet");
    }

    component.addElevatedOperation("Execute", "@TargetDir@\\AutoUpdater.exe", "-install", "UNDOEXECUTE", "@TargetDir@\\AutoUpdater.exe", "-remove");
}
/*
function Controller()
{
    installer.installationFinished.connect(function() {

        var isUpdate = installer.isUpdater();

        if(isUpdate)
        {
             var targetDir = installer.value("TargetDir");
             console.log("targetDir: " + targetDir);
             //installer.executeDetached(targetDir+"/AutoUpdater.exe --install");

        }else{
            var result = QMessageBox.question("quit.question", "Start Program", "Do you want to start the installed application?",QMessageBox.Yes | QMessageBox.No);
            if( result == QMessageBox.Yes)
            {
                var targetDir = installer.value("TargetDir");
                console.log("targetDir: " + targetDir);
                console.log("Is Updater: " + installer.isUpdater());
                console.log("Is Uninstaller: " + installer.isUninstaller());
                console.log("Is Package Manager: " + installer.isPackageManager());
                installer.executeDetached(targetDir+"/AutoUpdater.exe --install");
            }
        }
        });
    installer.updateFinished.connect(function(){
            var targetDir = installer.value("TargetDir");
            console.log("targetDir: " + targetDir);
            installer.executeDetached(targetDir+"/AutoUpdater.exe --install");
    });
}
*/
