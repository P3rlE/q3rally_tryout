"""Send one held ESC only to the explicitly identified foreground test client."""
import ctypes,sys,time
from ctypes import wintypes
u=ctypes.windll.user32;k=ctypes.windll.kernel32
u.GetForegroundWindow.restype=wintypes.HWND
u.GetWindowThreadProcessId.argtypes=[wintypes.HWND,ctypes.POINTER(wintypes.DWORD)]
u.SetForegroundWindow.argtypes=[wintypes.HWND]
u.BringWindowToTop.argtypes=[wintypes.HWND]
windows=[]
@ctypes.WINFUNCTYPE(wintypes.BOOL,wintypes.HWND,wintypes.LPARAM)
def visit(hwnd,param):
    pid=wintypes.DWORD();u.GetWindowThreadProcessId(hwnd,ctypes.byref(pid))
    if pid.value==int(sys.argv[1]) and u.IsWindowVisible(hwnd):windows.append(hwnd)
    return True
u.EnumWindows(visit,0)
if len(windows)!=1:raise SystemExit('Expected one test-game window')
thread=u.GetWindowThreadProcessId(u.GetForegroundWindow(),None);mine=k.GetCurrentThreadId()
u.AttachThreadInput(mine,thread,True)
try:u.BringWindowToTop(windows[0]);u.SetForegroundWindow(windows[0])
finally:u.AttachThreadInput(mine,thread,False)
time.sleep(.15)
pid=wintypes.DWORD();u.GetWindowThreadProcessId(u.GetForegroundWindow(),ctypes.byref(pid))
if pid.value!=int(sys.argv[1]):raise SystemExit('Test game not foreground; no input sent')
u.keybd_event(27,1,0,0)
for _ in range(3):time.sleep(.12);u.keybd_event(27,1,0,0)
time.sleep(.12);u.keybd_event(27,1,2,0);time.sleep(.15)
