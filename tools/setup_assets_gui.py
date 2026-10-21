"""Small desktop front end for the same tested asset setup pipeline."""
from pathlib import Path
import queue,subprocess,sys,threading,tkinter as tk
from tkinter import filedialog,messagebox,ttk
ROOT=Path(__file__).resolve().parents[1]
def main():
 window=tk.Tk();window.title("Ultrapool PSP - personal asset setup");window.geometry("790x610")
 frame=ttk.Frame(window,padding=16);frame.pack(fill="both",expand=True)
 ttk.Label(frame,text="Ultrapool PSP - prototype",font=("Segoe UI",16,"bold")).pack(anchor="w")
 ttk.Label(frame,text="Use your own Windows game or a complete GDRE-recovered project.\nConverted assets are private; do not redistribute them.").pack(anchor="w",pady=(4,12))
 mode=tk.StringVar(value="source");choice=ttk.Frame(frame);choice.pack(fill="x")
 ttk.Radiobutton(choice,text="Recovered project folder",variable=mode,value="source").pack(side="left")
 ttk.Radiobutton(choice,text="Original EXE / PCK + GDRE Tools",variable=mode,value="exe").pack(side="left",padx=12)
 fields={}
 def row(label,key,kind,default=""):
  line=ttk.Frame(frame);line.pack(fill="x",pady=4)
  ttk.Label(line,text=label,width=22).pack(side="left")
  var=tk.StringVar(value=default);fields[key]=var;ttk.Entry(line,textvariable=var).pack(side="left",fill="x",expand=True)
  def browse():
   if kind=="folder":value=filedialog.askdirectory()
   elif kind=="output":
    parent=filedialog.askdirectory(title="Choose parent folder for a NEW Ultrapool installation")
    value=str(Path(parent)/"Ultrapool") if parent else ""
   else:value=filedialog.askopenfilename()
   if value:var.set(value)
  ttk.Button(line,text="Browse...",command=browse).pack(side="left",padx=(6,0))
 row("Recovered project", "source","folder")
 row("Original game EXE / PCK","exe","file")
 row("GDRE Tools executable","gdre","file")
 row("FFmpeg executable","ffmpeg","file","ffmpeg")
 row("New installation folder","output","output",str(ROOT/"output/Ultrapool"))
 ttk.Label(frame,text="Requires Python packages from tools/requirements.txt and FFmpeg.\nThe output must be a new folder. Existing installations and saves are preserved.").pack(anchor="w",pady=8)
 log=tk.Text(frame,height=13,wrap="word",state="disabled");log.pack(fill="both",expand=True)
 events=queue.Queue();running=False
 def start():
  nonlocal running
  if running:return
  values={k:v.get().strip() for k,v in fields.items()};selected=mode.get()
  if not values[selected] or not values["output"] or (selected=="exe" and not values["gdre"]):
   messagebox.showerror("Missing input","Select your input and a new output folder. EXE/PCK mode also needs GDRE Tools.");return
  command=[sys.executable,"-u",str(ROOT/"tools/setup_assets.py"),"--"+selected,values[selected],"--output",values["output"],"--ffmpeg",values["ffmpeg"] or "ffmpeg"]
  if selected=="exe":command += ["--gdre",values["gdre"]]
  running=True;button.configure(state="disabled")
  def worker():
   try:
    with subprocess.Popen(command,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,text=True,encoding="utf-8",errors="replace",creationflags=getattr(subprocess,"CREATE_NO_WINDOW",0)) as process:
     for line in process.stdout:events.put(line)
     events.put((process.wait(),values["output"]))
   except Exception as e:events.put(str(e)+"\n");events.put((1,values["output"]))
  threading.Thread(target=worker,daemon=True).start()
 button=ttk.Button(frame,text="Build my PSP installation",command=start);button.pack(pady=(12,0))
 def poll():
  nonlocal running
  while not events.empty():
   item=events.get()
   if isinstance(item,tuple):
    running=False;button.configure(state="normal")
    if item[0]==0:messagebox.showinfo("Ready","Copy this folder into PSP/GAME:\n"+item[1])
    else:messagebox.showerror("Setup failed","See the log above. Your existing installation was preserved.")
   else:log.configure(state="normal");log.insert("end",item);log.see("end");log.configure(state="disabled")
  window.after(100,poll)
 def close():
  if running:messagebox.showinfo("Conversion running","Please wait for conversion to finish before closing.")
  else:window.destroy()
 window.protocol("WM_DELETE_WINDOW",close);poll();window.mainloop()
if __name__=="__main__":main()
