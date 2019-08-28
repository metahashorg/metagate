function Component()
{
    // default constructor
}

/*
function Controller()
{
}
*/

Component.prototype.beginInstallation = function()
{
    if (systemInfo.productType === "windows") {
        installer.setValue("RunProgram", "@TargetDir@/MetaGate.exe");
    } else if (systemInfo.productType === "osx") {
        installer.setValue("RunProgram", "@TargetDir@/MetaGate.app/Contents/MacOS/MetaGate");
    } else if (systemInfo.kernelType === "linux") {
        installer.setValue("RunProgram", "@TargetDir@/run.sh");
    }
}

Component.prototype.createOperations = function()
{
    // call default implementation to actually install MetaGate.exe
    component.createOperations();

    if (systemInfo.productType === "windows") {
	var mgpath = installer.value("TargetDir");
	mgpath = mgpath.replace(/\//g, "\\");

        component.addOperation("CreateShortcut", "@TargetDir@/MetaGate.exe", "@StartMenuDir@/MetaGate.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/MetaGate.exe",
            "iconId=0", "description=Launch Metahash Wallet");
        component.addOperation("CreateShortcut", "@TargetDir@/MetaGate.exe", "@DesktopDir@/MetaGate.lnk",
                "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/MetaGate.exe",
                "iconId=0", "description=Launch Metahash Wallet");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay", "Default", "URL:Mh pay protocol");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay", "URL Protocol", "");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay\\shell\\open\\command", "Default", "\"" + mgpath + "\\MetaGate.exe\" \"%1\"");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay\\DefaultIcon", "Default", mgpath+ "\\MetaGate.exe,1");
        component.addElevatedOperation("GlobalConfig", "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "MetaGate", "\"" + mgpath + "\\MetaGate.exe\" \"-t\"");
        // Autostart proxy
	component.addElevatedOperation("Execute", "@TargetDir@\\disablessdp.cmd", mgpath + "\\mhdesktopproxyservice.exe");
        component.addElevatedOperation("Execute", "@TargetDir@\\mhdesktopproxyservice.exe", "-i", "UNDOEXECUTE", "@TargetDir@\\mhdesktopproxyservice.exe", "-u");
        component.addElevatedOperation("Execute", "@TargetDir@\\mhdesktopproxyservice.exe", "", "UNDOEXECUTE", "@TargetDir@\\mhdesktopproxyservice.exe", "-t");
    } else if (systemInfo.productType === "osx") {
        component.addElevatedOperation("Execute", "@TargetDir@/launchd-install.sh", "metahash.desktopproxy", "@TargetDir@/MetaGate.app/Contents/MacOS/mhdesktopproxyservice", "UNDOEXECUTE", "rm", "-f", "/Library/LaunchDaemons/metahash.desktopproxy.plist");
    component.addElevatedOperation("Execute", "launchctl", "load", "/Library/LaunchDaemons/metahash.desktopproxy.plist", "UNDOEXECUTE", "launchctl", "unload", "/Library/LaunchDaemons/metahash.desktopproxy.plist");
    component.addElevatedOperation("Execute", "launchctl", "start", "metahash.desktopproxy", "UNDOEXECUTE", "launchctl", "stop", "metahash.desktopproxy");
        
    } else if (systemInfo.kernelType === "linux") {
        //component.addElevatedOperation("Execute", "@TargetDir@/install.sh", "UNDOEXECUTE", "@TargetDir@/uninstall.sh");
        component.addElevatedOperation("Execute", "@TargetDir@/systemd-install.sh", "metahash.desktopproxy", "@TargetDir@", "mhdesktopproxyservice.sh", "UNDOEXECUTE", "rm", "/etc/systemd/system/metahash.desktopproxy.service");
        component.addElevatedOperation("Execute", "systemctl", "enable", "metahash.desktopproxy", "UNDOEXECUTE", "systemctl", "disable", "metahash.desktopproxy");
        component.addElevatedOperation("Execute", "systemctl", "start", "metahash.desktopproxy", "UNDOEXECUTE", "systemctl", "stop", "metahash.desktopproxy");
    }

}

/*
Component.prototype.IntroductionPageCallback = function() {
    var result = QMessageBox.question("quit.question", "Start Program", "Do you want to start the installed application?",QMessageBox.Yes | QMessageBox.No);
   // if (installer.isUpdater()) {
         if (installer.isProcessRunning("MetaGate.exe") || installer.isProcessRunning("MetaGate")) {
             var widget =gui.currentPageWidget();
             widget.ErrorLabel.setText("<font color='red'>" + "Process: " + "YOUR_PROCESS" + " still running." + "</font>");

            // hide all following pages
            installer.setDefaultPageVisible(QInstaller.TargetDirectory, false);
            installer.setDefaultPageVisible(QInstaller.ComponentSelection, false);
            installer.setDefaultPageVisible(QInstaller.ReadyForInstallation, false);
            installer.setDefaultPageVisible(QInstaller.StartMenuSelection, false);
            installer.setDefaultPageVisible(QInstaller.PerformInstallation, false);
            installer.setDefaultPageVisible(QInstaller.LicenseCheck, false);
        }
    // }
}

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
                //installer.executeDetached(targetDir+"/AutoUpdater.exe --install");
            }
        }
        });
    installer.updateFinished.connect(function(){
            var targetDir = installer.value("TargetDir");
            console.log("targetDir: " + targetDir);
            //installer.executeDetached(targetDir+"/AutoUpdater.exe --install");
    });
}
*/
