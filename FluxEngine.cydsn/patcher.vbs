const READ = 1
const WRITE = 2

filename = "Generated_Source\PSoC5\USBFS_descr.c"
set fso = CreateObject("Scripting.FileSystemObject")

set file = fso.OpenTextFile(filename, READ)
text = file.ReadAll
file.Close

set r = New RegExp
r.MultiLine = True
r.Pattern = "/\* +compatibleID.*\n.*"
text = r.replace(text, "'W', 'I', 'N', 'U', 'S', 'B', 0, 0,")

set file = fso.CreateTextFile(filename, True)
file.Write text
file.Close
