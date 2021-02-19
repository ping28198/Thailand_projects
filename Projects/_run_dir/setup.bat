cmdkey /add:192.168.21.20 /user:Administrator /pass:THP@12345
cmdkey /add:192.168.21.21 /user:Administrator /pass:THP@12345

set exepath=%~dp0THP_OCR.exe



::@echo off
echo geneting short cut
::设置程序或文件的完整路径（必选）
set Program=%exepath%
   
::设置快捷方式名称（必选）
set LnkName=ocr_program
 
::设置程序的工作路径，一般为程序主目录，此项若留空，脚本将自行分析路径
set WorkDir=%~dp0
 
::设置快捷方式显示的说明（可选）
set Desc=ocr_program
 
if not defined WorkDir call:GetWorkDir "%Program%"
(echo Set WshShell=CreateObject("WScript.Shell"^)
echo strDesKtop=WshShell.SpecialFolders("DesKtop"^)
echo Set oShellLink=WshShell.CreateShortcut(strDesKtop^&"\%LnkName%.lnk"^)
echo oShellLink.TargetPath="%Program%"
echo oShellLink.WorkingDirectory="%WorkDir%"
echo oShellLink.WindowStyle=1
echo oShellLink.Description="%Desc%"
echo oShellLink.Save)>makelnk.vbs
echo link create success!
makelnk.vbs
del /f /q makelnk.vbs



::拷贝快捷方式到启动
set startupdir=%USERPROFILE%\AppData\Roaming\Microsoft\Windows\Start Menu\Programs\Startup\
set srcpath=%USERPROFILE%\desktop\%LnkName%.lnk
copy "%srcpath%" "%startupdir%"


::设置防火墙例外
netsh advfirewall firewall add rule name="THP_OCR" dir=in program="%exepath%" security=authnoencap action=allow



exit
goto :eof
:GetWorkDir
set WorkDir=%~dp1
set WorkDir=%WorkDir:~,-1%
goto :eof
