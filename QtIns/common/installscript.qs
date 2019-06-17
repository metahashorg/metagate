function Component()
{
    // default constructor
}

function Controller()
{
}

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
        component.addOperation("CreateShortcut", "@TargetDir@/MetaGate.exe", "@StartMenuDir@/MetaGate.lnk",
            "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/MetaGate.exe",
            "iconId=0", "description=Launch Metahash Wallet");
        component.addOperation("CreateShortcut", "@TargetDir@/MetaGate.exe", "@DesktopDir@/MetaGate.lnk",
                "workingDirectory=@TargetDir@", "iconPath=@TargetDir@/MetaGate.exe",
                "iconId=0", "description=Launch Metahash Wallet");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay", "Default", "URL:Mh pay protocol");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay", "URL Protocol", "");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay\\shell\\open\\command", "Default", "\"@TargetDir@\\MetaGate.exe\" \"%1\"");
        component.addElevatedOperation("GlobalConfig", "HKEY_CLASSES_ROOT\\metapay\\DefaultIcon", "Default", "@TargetDir@\\MetaGate.exe,1");
    //component.addElevatedOperation("GlobalConfig", "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "MetaGate", "\"@TargetDir@\\MetaGate.exe\" \"-t\"");
        component.addElevatedOperation("GlobalConfig", "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run", "MetaGate", "\"@TargetDir@\\MetaGate.exe\" \"-t\"");
        // Autostart proxy
        component.addElevatedOperation("Execute", "@TargetDir@\\asdesktopproxyservice.exe", "-i", "UNDOEXECUTE", "@TargetDir@\\asdesktopproxyservice.exe", "-u");
        component.addElevatedOperation("Execute", "@TargetDir@\\asdesktopproxyservice.exe", "", "UNDOEXECUTE", "@TargetDir@\\asdesktopproxyservice.exe", "-t");
    } else if (systemInfo.productType === "osx") {
        component.addElevatedOperation("Execute", "@TargetDir@/launchd-install.sh", "com.adsniper.desktopproxy", "@TargetDir@/MetaGate.app/Contents/MacOS/asdesktopproxyservice", "UNDOEXECUTE", "rm", "-f", "/Library/LaunchDaemons/com.adsniper.desktopproxy.plist");
    component.addElevatedOperation("Execute", "launchctl", "load", "/Library/LaunchDaemons/com.adsniper.desktopproxy.plist", "UNDOEXECUTE", "launchctl", "unload", "/Library/LaunchDaemons/com.adsniper.desktopproxy.plist");
    component.addElevatedOperation("Execute", "launchctl", "start", "com.adsniper.desktopproxy", "UNDOEXECUTE", "launchctl", "stop", "com.adsniper.desktopproxy");
        
    } else if (systemInfo.kernelType === "linux") {
        component.createOperations();
        component.addElevatedOperation("Execute", "@TargetDir@/systemd-install.sh", "com.adsniper.desktopproxy", "@TargetDir@", "asdesktopproxyservice", "UNDOEXECUTE", "rm", "/etc/systemd/system/com.adsniper.desktopproxy.service");
        component.addElevatedOperation("Execute", "systemctl", "enable", "com.adsniper.desktopproxy", "UNDOEXECUTE", "systemctl", "disable", "com.adsniper.desktopproxy");
        component.addElevatedOperation("Execute", "systemctl", "start", "com.adsniper.desktopproxy", "UNDOEXECUTE", "systemctl", "stop", "com.adsniper.desktopproxy");
        
    }

    //component.addElevatedOperation("Execute", "@TargetDir@\\AutoUpdater.exe", "-install", "UNDOEXECUTE", "@TargetDir@\\AutoUpdater.exe", "-remove");
}

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
