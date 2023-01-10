# StartSuspended
StartSuspended is a highest level kernel mode utility driver that suspends a predefined process at creation.
## Use case
Sometimes it is easier or needed to attach a debugger to a process because some other process is starting it. One could use this driver to prevent the created process from executing any code, attaching a debugger and resuming the process.
## How to use
Only official signed kernel drivers are allowed to be installed on a Windows machine. To be able to use this driver you need to disable the enforcement of signature verification and enable the Windows test mode.
### Disable driver signature enforcement
To disable driver signature enforcement run the following command in a command line window with administrative privileges:
```shell
bcdedit /set testsigning on
```
You need to disable Secureboot in your BIOS/VM settings to enable test signing.
### Create the driver
It is necessary to build the driver once the repository was cloned. After that one can create the driver with:
```shell
sc create [serviceName] binPath= [absolute path of the build .sys file] type= kernel
```
The spaces after the argument keys are mandatory.
### Configuring the driver
To tell the driver what process to suspend, one must add the Target value with REG_SZ type to the registry key of the previous created driver.
The registry key is usually found at the following registry path:
```shell
HKLM\SYSTEM\ControlSet001\Services\[serviceName]
```
To for example suspend every notepad instance on creation one would add a registry value called "Target" with the REG_SZ data "Notepad.exe".
### Starting/Stopping the driver
To start/stop the driver run the following command in a command line window with administrative privileges:
```shell
sc start [serviceName]
sc stop [serviceName]
```
The driver runs with SERVICE_DEMAND_START, which means you always need to start the driver manually.
### Continuing the suspended process
It is possible to continue the suspended process by using the Windows resource monitor.
### Debugging possible errors on driver start
The driver logs every error using the [KdPrintEx](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/wdm/nf-wdm-kdprintex) macro. The log messages can be viewed using [DebugView](https://learn.microsoft.com/en-us/sysinternals/downloads/debugview) with kernel capture on and verbose output enabled.
## How it works
The driver makes use of the [PsSetCreateProcessNotifyRoutineEx](https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntddk/nf-ntddk-pssetcreateprocessnotifyroutineex) function to intercept any process creation on the system and compares the name to the registry key entry. If both match it uses the undocumented [PsSuspendProcess](https://www.geoffchappell.com/studies/windows/km/ntoskrnl/history/names60.htm) function to suspend the process.
## Remarks
The driver runs in the highest level mode and therefore it is possible to suspend nearly any process one would think of. Be careful.
