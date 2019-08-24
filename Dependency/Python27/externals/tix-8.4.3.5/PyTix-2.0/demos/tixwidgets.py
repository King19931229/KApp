#! /usr/local/bin/python
#
# $Id: tixwidgets.py,v 1.2 2000/11/22 08:03:01 idiscovery Exp $
#
# tixwidgets.py --
# 	This is a demo program of all Tix widgets available from Python. If
#	you have installed Python & Tix properly, you can execute this as
#
#		% tixwidget.py
#

import os, sys, Tix

class Demo:
    pass

root = Tix.Tk()
demo = Demo()
demo.dir = None				# script directory
demo.balloon = None			# balloon widget
demo.useBalloons = Tix.StringVar()
demo.useBalloons.set('0')
demo.statusbar = None			# status bar widget
demo.welmsg = None			# Msg widget
demo.welfont = ''			# font name
demo.welsize = ''			# font size

def main():
    global demo, root

    progname = sys.argv[0]
    dirname = os.path.dirname(progname)
    if dirname and dirname != os.curdir:
	demo.dir = dirname
	index = -1
	for i in range(len(sys.path)):
	    p = sys.path[i]
	    if p in ("", os.curdir):
		index = i
	if index >= 0:
	    sys.path[index] = dirname
	else:
	    sys.path.insert(0, dirname)
    else:
	demo.dir = os.getcwd()
    sys.path.insert(0, demo.dir+'/samples')

    root.withdraw()
    root = Tix.Toplevel()
    root.title('Tix Widget Demonstration')
    root.geometry('780x570+50+50')

    demo.balloon = Tix.Balloon(root)
    frame1 = MkMainMenu(root)
    frame2 = MkMainNotebook(root)
    frame3 = MkMainStatus(root)
    frame1.pack(side=Tix.TOP, fill=Tix.X)
    frame3.pack(side=Tix.BOTTOM, fill=Tix.X)
    frame2.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH, padx=4, pady=4)
    demo.balloon['statusbar'] = demo.statusbar
    root.mainloop()

def exit_cmd(event=None):
    sys.exit()

def MkMainMenu(top):
    global demo

    w = Tix.Frame(top, bd=2, relief=Tix.RAISED)
    file = Tix.Menubutton(w, text='File', underline=0, takefocus=0)
    help = Tix.Menubutton(w, text='Help', underline=0, takefocus=0)
    file.pack(side=Tix.LEFT)
    help.pack(side=Tix.RIGHT)
    fm = Tix.Menu(file)
    file['menu'] = fm
    hm = Tix.Menu(help)
    help['menu'] = hm

    fm.add_command(label='Exit', underline=1, accelerator='Ctrl+X',
		   command=exit_cmd)
    hm.add_checkbutton(label='BalloonHelp', underline=0, command=ToggleHelp,
		       variable=demo.useBalloons)
    # The trace variable option doesn't seem to work, instead I use 'command'
    #apply(w.tk.call, ('trace', 'variable', demo.useBalloons, 'w',
    #		      ToggleHelp))
    top.bind_all("<Control-x>", exit_cmd)
    top.bind_all("<Control-X>", exit_cmd)
    return w

def MkMainNotebook(top):
    top.option_add('*TixNoteBook*tagPadX', 6)
    top.option_add('*TixNoteBook*tagPadY', 4)
    top.option_add('*TixNoteBook*borderWidth', 2)
    top.option_add('*TixNoteBook*font',
		   '-*-helvetica-bold-o-normal-*-14-*-*-*-*-*-*-*')
    w = Tix.NoteBook(top, ipadx=5, ipady=5)
    w.add('wel', label='Welcome', underline=0,
	  createcmd=lambda w=w, name='wel': MkWelcome(w, name))
    w.add('cho', label='Choosers', underline=0,
	  createcmd=lambda w=w, name='cho': MkChoosers(w, name))
    w.add('scr', label='Scrolled Widgets', underline=0,
	  createcmd=lambda w=w, name='scr': MkScroll(w, name))
    w.add('mgr', label='Manager Widgets', underline=0,
	  createcmd=lambda w=w, name='mgr': MkManager(w, name))
    w.add('dir', label='Directory List', underline=0,
	  createcmd=lambda w=w, name='dir': MkDirList(w, name))
    w.add('exp', label='Run Sample Programs', underline=0,
	  createcmd=lambda w=w, name='exp': MkSample(w, name))
    return w

def MkMainStatus(top):
    global demo

    w = Tix.Frame(top, relief=Tix.RAISED, bd=1)
    demo.statusbar = Tix.Label(w, relief=Tix.SUNKEN, bd=1, font='-*-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*')
    demo.statusbar.form(padx=3, pady=3, left=0, right='%70')
    return w

def MkWelcome(nb, name):
    w = nb.page(name)
    bar = MkWelcomeBar(w)
    text = MkWelcomeText(w)
    bar.pack(side=Tix.TOP, fill=Tix.X, padx=2, pady=2)
    text.pack(side=Tix.TOP, fill=Tix.BOTH, expand=1)

def MkWelcomeBar(top):
    global demo

    w = Tix.Frame(top, bd=2, relief=Tix.GROOVE)
    b1 = Tix.ComboBox(w, command=lambda w=top: MainTextFont(w))
    b2 = Tix.ComboBox(w, command=lambda w=top: MainTextFont(w))
    b1.entry['width'] = 15
    b1.slistbox.listbox['height'] = 3
    b2.entry['width'] = 4
    b2.slistbox.listbox['height'] = 3

    demo.welfont = b1
    demo.welsize = b2

    b1.insert(Tix.END, 'Courier')
    b1.insert(Tix.END, 'Helvetica')
    b1.insert(Tix.END, 'Lucida')
    b1.insert(Tix.END, 'Times Roman')

    b2.insert(Tix.END, '8')
    b2.insert(Tix.END, '10')
    b2.insert(Tix.END, '12')
    b2.insert(Tix.END, '14')
    b2.insert(Tix.END, '18')

    b1.pick(1)
    b2.pick(3)

    b1.pack(side=Tix.LEFT, padx=4, pady=4)
    b2.pack(side=Tix.LEFT, padx=4, pady=4)

    demo.balloon.bind_widget(b1, msg='Choose\na font',
			     statusmsg='Choose a font for this page')
    demo.balloon.bind_widget(b2, msg='Point size',
			     statusmsg='Choose the font size for this page')
    return w

def MkWelcomeText(top):
    global demo

    w = Tix.ScrolledWindow(top, scrollbar='auto')
    win = w.window
    title = Tix.Label(win, font='-*-times-bold-r-normal-*-18-*-*-*-*-*-*-*',
		      bd=0, width=30, anchor=Tix.N, text='Welcome to TIX Version 4.0 from Python Version 1.3')
    msg = Tix.Message(win, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      bd=0, width=400, anchor=Tix.N,
		      text='Tix 4.0 is a set of mega-widgets based on TK. This program \
demonstrates the widgets in the Tix widget set. You can choose the pages \
in this window to look at the corresponding widgets. \n\n\
To quit this program, choose the "File | Exit" command.')
    title.pack(expand=1, fill=Tix.BOTH, padx=10, pady=10)
    msg.pack(expand=1, fill=Tix.BOTH, padx=10, pady=10)
    demo.welmsg = msg
    return w

def MainTextFont(w):
    global demo

    if not demo.welmsg:
	return
    font = demo.welfont['value']
    point = demo.welsize['value']
    if font == 'Times Roman':
	font = 'times'
    fontstr = '-*-%s-bold-r-normal-*-%s-*-*-*-*-*-*-*' % (font, point)
    demo.welmsg['font'] = fontstr

def ToggleHelp():
    if demo.useBalloons.get() == '1':
	demo.balloon['state'] = 'both'
    else:
	demo.balloon['state'] = 'none'

def MkChoosers(nb, name):
    w = nb.page(name)
    prefix = Tix.OptionName(w)
    if not prefix:
	prefix = ''
    w.option_add('*' + prefix + '*TixLabelFrame*label.padX', 4)

    til = Tix.LabelFrame(w, label='Chooser Widgets')
    cbx = Tix.LabelFrame(w, label='tixComboBox')
    ctl = Tix.LabelFrame(w, label='tixControl')
    sel = Tix.LabelFrame(w, label='tixSelect')
    opt = Tix.LabelFrame(w, label='tixOptionMenu')
    fil = Tix.LabelFrame(w, label='tixFileEntry')
    fbx = Tix.LabelFrame(w, label='tixFileSelectBox')
    tbr = Tix.LabelFrame(w, label='Tool Bar')

    MkTitle(til.frame)
    MkCombo(cbx.frame)
    MkControl(ctl.frame)
    MkSelect(sel.frame)
    MkOptMenu(opt.frame)
    MkFileEnt(fil.frame)
    MkFileBox(fbx.frame)
    MkToolBar(tbr.frame)

    # First column: comBox and selector
    cbx.form(top=0, left=0, right='%33')
    sel.form(left=0, right='&'+str(cbx), top=cbx)
    opt.form(left=0, right='&'+str(cbx), top=sel, bottom=-1)

    # Second column: title .. etc
    til.form(left=cbx, top=0,right='%66')
    ctl.form(left=cbx, right='&'+str(til), top=til)
    fil.form(left=cbx, right='&'+str(til), top=ctl)
    tbr.form(left=cbx, right='&'+str(til), top=fil, bottom=-1)

    #
    # Third column: file selection
    fbx.form(right=-1, top=0, left='%66')

def MkCombo(w):
    prefix = Tix.OptionName(w)
    if not prefix: prefix = ''
    w.option_add('*' + prefix + '*TixComboBox*label.width', 10)
    w.option_add('*' + prefix + '*TixComboBox*label.anchor', Tix.E)
    w.option_add('*' + prefix + '*TixComboBox*entry.width', 14)

    static = Tix.ComboBox(w, label='Static', editable=0)
    editable = Tix.ComboBox(w, label='Editable', editable=1)
    history = Tix.ComboBox(w, label='History', editable=1, history=1,
			   anchor=Tix.E)
    static.insert(Tix.END, 'January')
    static.insert(Tix.END, 'February')
    static.insert(Tix.END, 'March')
    static.insert(Tix.END, 'April')
    static.insert(Tix.END, 'May')
    static.insert(Tix.END, 'June')
    static.insert(Tix.END, 'July')
    static.insert(Tix.END, 'August')
    static.insert(Tix.END, 'September')
    static.insert(Tix.END, 'October')
    static.insert(Tix.END, 'November')
    static.insert(Tix.END, 'December')

    editable.insert(Tix.END, 'Angola')
    editable.insert(Tix.END, 'Bangladesh')
    editable.insert(Tix.END, 'China')
    editable.insert(Tix.END, 'Denmark')
    editable.insert(Tix.END, 'Ecuador')

    history.insert(Tix.END, '/usr/bin/ksh')
    history.insert(Tix.END, '/usr/local/lib/python')
    history.insert(Tix.END, '/var/adm')

    static.pack(side=Tix.TOP, padx=5, pady=3)
    editable.pack(side=Tix.TOP, padx=5, pady=3)
    history.pack(side=Tix.TOP, padx=5, pady=3)

states = ['Bengal', 'Delhi', 'Karnataka', 'Tamil Nadu']

def spin_cmd(w, inc):
    idx = states.index(demo_spintxt.get()) + inc
    if idx < 0:
	idx = len(states) - 1
    elif idx >= len(states):
	idx = 0
# following doesn't work.
#    return states[idx]
    demo_spintxt.set(states[idx])	# this works

def spin_validate(w):
    global states, demo_spintxt

    try:
	i = states.index(demo_spintxt.get())
    except:
	return states[0]
    return states[i]
    # why this procedure works as opposed to the previous one beats me.

def MkControl(w):
    global demo_spintxt

    prefix = Tix.OptionName(w)
    if not prefix: prefix = ''
    w.option_add('*' + prefix + '*TixControl*label.width', 10)
    w.option_add('*' + prefix + '*TixControl*label.anchor', Tix.E)
    w.option_add('*' + prefix + '*TixControl*entry.width', 13)

    demo_spintxt = Tix.StringVar()
    demo_spintxt.set(states[0])
    simple = Tix.Control(w, label='Numbers')
    spintxt = Tix.Control(w, label='States', variable=demo_spintxt)
    spintxt['incrcmd'] = lambda w=spintxt: spin_cmd(w, 1)
    spintxt['decrcmd'] = lambda w=spintxt: spin_cmd(w, -1)
    spintxt['validatecmd'] = lambda w=spintxt: spin_validate(w)

    simple.pack(side=Tix.TOP, padx=5, pady=3)
    spintxt.pack(side=Tix.TOP, padx=5, pady=3)
    
def MkSelect(w):
    prefix = Tix.OptionName(w)
    if not prefix: prefix = ''
    w.option_add('*' + prefix + '*TixSelect*label.anchor', Tix.CENTER)
    w.option_add('*' + prefix + '*TixSelect*orientation', Tix.VERTICAL)
    w.option_add('*' + prefix + '*TixSelect*labelSide', Tix.TOP)

    sel1 = Tix.Select(w, label='Mere Mortals', allowzero=1, radio=1)
    sel2 = Tix.Select(w, label='Geeks', allowzero=1, radio=0)

    sel1.add('eat', text='Eat')
    sel1.add('work', text='Work')
    sel1.add('play', text='Play')
    sel1.add('party', text='Party')
    sel1.add('sleep', text='Sleep')

    sel2.add('eat', text='Eat')
    sel2.add('prog1', text='Program')
    sel2.add('prog2', text='Program')
    sel2.add('prog3', text='Program')
    sel2.add('sleep', text='Sleep')

    sel1.pack(side=Tix.LEFT, padx=5, pady=3, fill=Tix.X)
    sel2.pack(side=Tix.LEFT, padx=5, pady=3, fill=Tix.X)

def MkOptMenu(w):
    prefix = Tix.OptionName(w)
    if not prefix: prefix = ''
    w.option_add('*' + prefix + '*TixOptionMenu*label.anchor', Tix.E)
    m = Tix.OptionMenu(w, label='File Format : ', options='menubutton.width 15')
    m.add_command('text', label='Plain Text')
    m.add_command('post', label='PostScript')
    m.add_command('format', label='Formatted Text')
    m.add_command('html', label='HTML')
    m.add_command('sep')
    m.add_command('tex', label='LaTeX')
    m.add_command('rtf', label='Rich Text Format')

    m.pack(fill=Tix.X, padx=5, pady=3)

def MkFileEnt(w):
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='Press the "open file" icon button and a TixFileSelectDialog will popup.')
    ent = Tix.FileEntry(w, label='Select a file : ')
    msg.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH, padx=3, pady=3)
    ent.pack(side=Tix.TOP, fill=Tix.X, padx=3, pady=3)

def MkFileBox(w):
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='The TixFileSelectBox is a Motif-style box with various enhancements. For example, you can adjust the size of the two listboxes and your past selections are recorded.')
    box = Tix.FileSelectBox(w)
    msg.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH, padx=3, pady=3)
    box.pack(side=Tix.TOP, fill=Tix.X, padx=3, pady=3)

def MkToolBar(w):
    global demo

    prefix = Tix.OptionName(w)
    if not prefix: prefix = ''
    w.option_add('*' + prefix + '*TixSelect*frame.borderWidth', 1)
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='The Select widget is also good for arranging buttons in a tool bar.')
    bar = Tix.Frame(w, bd=2, relief=Tix.RAISED)
    font = Tix.Select(w, allowzero=1, radio=0, label='')
    para = Tix.Select(w, allowzero=0, radio=1, label='')

    font.add('bold', bitmap='@' + demo.dir + '/bitmaps/bold.xbm')
    font.add('italic', bitmap='@' + demo.dir + '/bitmaps/italic.xbm')
    font.add('underline', bitmap='@' + demo.dir + '/bitmaps/underline.xbm')
    font.add('capital', bitmap='@' + demo.dir + '/bitmaps/capital.xbm')

    para.add('left', bitmap='@' + demo.dir + '/bitmaps/leftj.xbm')
    para.add('right', bitmap='@' + demo.dir + '/bitmaps/rightj.xbm')
    para.add('center', bitmap='@' + demo.dir + '/bitmaps/centerj.xbm')
    para.add('justify', bitmap='@' + demo.dir + '/bitmaps/justify.xbm')

    msg.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH, padx=3, pady=3)
    bar.pack(side=Tix.TOP, fill=Tix.X, padx=3, pady=3)
    font.pack({'in':bar}, side=Tix.LEFT, padx=3, pady=3)
    para.pack({'in':bar}, side=Tix.LEFT, padx=3, pady=3)

def MkTitle(w):
    prefix = Tix.OptionName(w)
    if not prefix: prefix = ''
    w.option_add('*' + prefix + '*TixSelect*frame.borderWidth', 1)
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='There are many types of "chooser" widgets that allow the user to input different types of information')
    msg.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH, padx=3, pady=3)

def MkScroll(nb, name):
    w = nb.page(name)
    prefix = Tix.OptionName(w)
    if not prefix:
	prefix = ''
    w.option_add('*' + prefix + '*TixLabelFrame*label.padX', 4)

    sls = Tix.LabelFrame(w, label='tixScrolledListBox')
    swn = Tix.LabelFrame(w, label='tixScrolledWindow')
    stx = Tix.LabelFrame(w, label='tixScrolledText')

    MkSList(sls.frame)
    MkSWindow(swn.frame)
    MkSText(stx.frame)

    sls.form(top=0, left=0, right='%33', bottom=-1)
    swn.form(top=0, left=sls, right='%66', bottom=-1)
    stx.form(top=0, left=swn, right=-1, bottom=-1)

def MkSList(w):
    top = Tix.Frame(w, width=300, height=330)
    bot = Tix.Frame(w)
    msg = Tix.Message(top, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=200, anchor=Tix.N,
		      text='This TixScrolledListBox is configured so that it uses scrollbars only when it is necessary. Use the handles to resize the listbox and watch the scrollbars automatically appear and disappear.')

    list = Tix.ScrolledListBox(top, scrollbar='auto')
    list.place(x=50, y=150, width=120, height=80)
    list.listbox.insert(Tix.END, 'Alabama')
    list.listbox.insert(Tix.END, 'California')
    list.listbox.insert(Tix.END, 'Montana')
    list.listbox.insert(Tix.END, 'New Jersey')
    list.listbox.insert(Tix.END, 'New York')
    list.listbox.insert(Tix.END, 'Pennsylvania')
    list.listbox.insert(Tix.END, 'Washington')

    rh = Tix.ResizeHandle(top, bg='black',
			  relief=Tix.RAISED,
			  handlesize=8, gridded=1, minwidth=50, minheight=30)
    btn = Tix.Button(bot, text='Reset', command=lambda w=rh, x=list: SList_reset(w,x))
    top.propagate(0)
    msg.pack(fill=Tix.X)
    btn.pack(anchor=Tix.CENTER)
    top.pack(expand=1, fill=Tix.BOTH)
    bot.pack(fill=Tix.BOTH)
    list.bind('<Map>', func=lambda arg=0, rh=rh, list=list:
	      list.tk.call('tixDoWhenIdle', str(rh), 'attachwidget', str(list)))

def SList_reset(rh, list):
    list.place(x=50, y=150, width=120, height=80)
    list.update()
    rh.attach_widget(list)

def MkSWindow(w):
    global demo

    top = Tix.Frame(w, width=330, height=330)
    bot = Tix.Frame(w)
    msg = Tix.Message(top, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=200, anchor=Tix.N,
		      text='The TixScrolledWindow widget allows you to scroll any kind of Tk widget. It is more versatile than a scrolled canvas widget.')
    win = Tix.ScrolledWindow(top, scrollbar='auto')
    image = Tix.Image('photo', file=demo.dir + "/bitmaps/tix.gif")
    lbl = Tix.Label(win.window, image=image)
    lbl.pack(expand=1, fill=Tix.BOTH)

    win.place(x=30, y=150, width=190, height=120)

    rh = Tix.ResizeHandle(top, bg='black',
			  relief=Tix.RAISED,
			  handlesize=8, gridded=1, minwidth=50, minheight=30)
    btn = Tix.Button(bot, text='Reset', command=lambda w=rh, x=win: SWindow_reset(w,x))
    top.propagate(0)
    msg.pack(fill=Tix.X)
    btn.pack(anchor=Tix.CENTER)
    top.pack(expand=1, fill=Tix.BOTH)
    bot.pack(fill=Tix.BOTH)
    win.bind('<Map>', func=lambda arg=0, rh=rh, win=win:
	     win.tk.call('tixDoWhenIdle', str(rh), 'attachwidget', str(win)))

def SWindow_reset(rh, win):
    win.place(x=30, y=150, width=190, height=120)
    win.update()
    rh.attach_widget(win)

def MkSText(w):
    top = Tix.Frame(w, width=330, height=330)
    bot = Tix.Frame(w)
    msg = Tix.Message(top, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=200, anchor=Tix.N,
		      text='The TixScrolledWindow widget allows you to scroll any kind of Tk widget. It is more versatile than a scrolled canvas widget.')

    win = Tix.ScrolledText(top, scrollbar='auto')
#    win.text['wrap'] = 'none'
    win.text.insert(Tix.END, 'This is a text widget embedded in a scrolled window. Although the original Tix demo does not have any text here, I decided to put in some so that you can see the effect of scrollbars etc.')
    win.place(x=30, y=150, width=190, height=100)

    rh = Tix.ResizeHandle(top, bg='black',
			  relief=Tix.RAISED,
			  handlesize=8, gridded=1, minwidth=50, minheight=30)
    btn = Tix.Button(bot, text='Reset', command=lambda w=rh, x=win: SText_reset(w,x))
    top.propagate(0)
    msg.pack(fill=Tix.X)
    btn.pack(anchor=Tix.CENTER)
    top.pack(expand=1, fill=Tix.BOTH)
    bot.pack(fill=Tix.BOTH)
    win.bind('<Map>', func=lambda arg=0, rh=rh, win=win:
	     win.tk.call('tixDoWhenIdle', str(rh), 'attachwidget', str(win)))

def SText_reset(rh, win):
    win.place(x=30, y=150, width=190, height=120)
    win.update()
    rh.attach_widget(win)

def MkManager(nb, name):
    w = nb.page(name)
    prefix = Tix.OptionName(w)
    if not prefix:
	prefix = ''
    w.option_add('*' + prefix + '*TixLabelFrame*label.padX', 4)

    pane = Tix.LabelFrame(w, label='tixPanedWindow')
    note = Tix.LabelFrame(w, label='tixNoteBook')

    MkPanedWindow(pane.frame)
    MkNoteBook(note.frame)

    pane.form(top=0, left=0, right=note, bottom=-1)
    note.form(top=0, right=-1, bottom=-1)

def MkPanedWindow(w):
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='The PanedWindow widget allows the user to interactively manipulate the sizes of several panes. The panes can be arranged either vertically or horizontally.')
    group = Tix.Label(w, text='Newsgroup: comp.lang.python')
    pane = Tix.PanedWindow(w, orientation='vertical')

    p1 = pane.add('list', min=70, size=100)
    p2 = pane.add('text', min=70)
    list = Tix.ScrolledListBox(p1)
    text = Tix.ScrolledText(p2)

    list.listbox.insert(Tix.END, "  12324 Re: TK is good for your health")
    list.listbox.insert(Tix.END, "+ 12325 Re: TK is good for your health")
    list.listbox.insert(Tix.END, "+ 12326 Re: Tix is even better for your health (Was: TK is good...)")
    list.listbox.insert(Tix.END, "  12327 Re: Tix is even better for your health (Was: TK is good...)")
    list.listbox.insert(Tix.END, "+ 12328 Re: Tix is even better for your health (Was: TK is good...)")
    list.listbox.insert(Tix.END, "  12329 Re: Tix is even better for your health (Was: TK is good...)")
    list.listbox.insert(Tix.END, "+ 12330 Re: Tix is even better for your health (Was: TK is good...)")

    text.text['bg'] = list.listbox['bg']
    text.text['wrap'] = 'none'
    text.text.insert(Tix.END, """
Mon, 19 Jun 1995 11:39:52        comp.lang.tcl              Thread   34 of  220
Lines 353       A new way to put text and bitmaps together iNo responses
ioi@blue.seas.upenn.edu                Ioi K. Lam at University of Pennsylvania

Hi,

I have implemented a new image type called "compound". It allows you
to glue together a bunch of bitmaps, images and text strings together
to form a bigger image. Then you can use this image with widgets that
support the -image option. This way you can display very fancy stuffs
in your GUI. For example, you can display a text string string
together with a bitmap, at the same time, inside a TK button widget. A
screenshot of compound images can be found at the bottom of this page:

        http://www.cis.upenn.edu/~ioi/tix/screenshot.html

You can also you is in other places such as putting fancy bitmap+text
in menus, tabs of tixNoteBook widgets, etc. This feature will be
included in the next release of Tix (4.0b1). Count on it to make jazzy
interfaces!""")
    list.pack(expand=1, fill=Tix.BOTH, padx=4, pady=6)
    text.pack(expand=1, fill=Tix.BOTH, padx=4, pady=6)

    msg.pack(side=Tix.TOP, padx=3, pady=3, fill=Tix.BOTH)
    group.pack(side=Tix.TOP, padx=3, pady=3, fill=Tix.BOTH)
    pane.pack(side=Tix.TOP, padx=3, pady=3, fill=Tix.BOTH, expand=1)

def MkNoteBook(w):
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='The NoteBook widget allows you to layout a complex interface into individual pages.')
    prefix = Tix.OptionName(w)
    if not prefix:
	prefix = ''
    w.option_add('*' + prefix + '*TixControl*entry.width', 10)
    w.option_add('*' + prefix + '*TixControl*label.width', 18)
    w.option_add('*' + prefix + '*TixControl*label.anchor', Tix.E)
    w.option_add('*' + prefix + '*TixNoteBook*tagPadX', 8)

    nb = Tix.NoteBook(w, ipadx=6, ipady=6)
    nb.add('hard_disk', label="Hard Disk", underline=0)
    nb.add('network', label="Network", underline=0)

    # Frame for the buttons that are present on all pages
    common = Tix.Frame(nb.hard_disk)
    common.pack(side=Tix.RIGHT, padx=2, pady=2, fill=Tix.Y)
    CreateCommonButtons(common)

    # Widgets belonging only to this page
    a = Tix.Control(nb.hard_disk, value=12, label='Access Time: ')
    w = Tix.Control(nb.hard_disk, value=400, label='Write Throughput: ')
    r = Tix.Control(nb.hard_disk, value=400, label='Read Throughput: ')
    c = Tix.Control(nb.hard_disk, value=1021, label='Capacity: ')
    a.pack(side=Tix.TOP, padx=20, pady=2)
    w.pack(side=Tix.TOP, padx=20, pady=2)
    r.pack(side=Tix.TOP, padx=20, pady=2)
    c.pack(side=Tix.TOP, padx=20, pady=2)

    common = Tix.Frame(nb.network)
    common.pack(side=Tix.RIGHT, padx=2, pady=2, fill=Tix.Y)
    CreateCommonButtons(common)

    a = Tix.Control(nb.network, value=12, label='Access Time: ')
    w = Tix.Control(nb.network, value=400, label='Write Throughput: ')
    r = Tix.Control(nb.network, value=400, label='Read Throughput: ')
    c = Tix.Control(nb.network, value=1021, label='Capacity: ')
    u = Tix.Control(nb.network, value=10, label='Users: ')
    a.pack(side=Tix.TOP, padx=20, pady=2)
    w.pack(side=Tix.TOP, padx=20, pady=2)
    r.pack(side=Tix.TOP, padx=20, pady=2)
    c.pack(side=Tix.TOP, padx=20, pady=2)
    u.pack(side=Tix.TOP, padx=20, pady=2)

    msg.pack(side=Tix.TOP, padx=3, pady=3, fill=Tix.BOTH)
    nb.pack(side=Tix.TOP, padx=5, pady=5, fill=Tix.BOTH, expand=1)

def CreateCommonButtons(f):
    ok = Tix.Button(f, text='OK', width = 6)
    cancel = Tix.Button(f, text='Cancel', width = 6)
    ok.pack(side=Tix.TOP, padx=2, pady=2)
    cancel.pack(side=Tix.TOP, padx=2, pady=2)

def MkDirList(nb, name):
    w = nb.page(name)
    prefix = Tix.OptionName(w)
    if not prefix:
	prefix = ''
    w.option_add('*' + prefix + '*TixLabelFrame*label.padX', 4)

    dir = Tix.LabelFrame(w, label='tixDirList')
    fsbox = Tix.LabelFrame(w, label='tixExFileSelectBox')
    MkDirListWidget(dir.frame)
    MkExFileWidget(fsbox.frame)
    dir.form(top=0, left=0, right='%40', bottom=-1)
    fsbox.form(top=0, left='%40', right=-1, bottom=-1)

def MkDirListWidget(w):
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='The TixDirList widget gives a graphical representation of the file system directory and makes it easy for the user to choose and access directories.')
    dirlist = Tix.DirList(w, options='hlist.padY 1 hlist.width 25 hlist.height 16')
    msg.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH, padx=3, pady=3)
    dirlist.pack(side=Tix.TOP, padx=3, pady=3)

def MkExFileWidget(w):
    msg = Tix.Message(w, font='-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*',
		      relief=Tix.FLAT, width=240, anchor=Tix.N,
		      text='The TixExFileSelectBox widget is more user friendly than the Motif style FileSelectBox.')
    # There's a bug in the ComboBoxes - the scrolledlistbox is destroyed
    box = Tix.ExFileSelectBox(w, bd=2, relief=Tix.RAISED)
    msg.pack(side=Tix.TOP, expand=1, fill=Tix.BOTH, padx=3, pady=3)
    box.pack(side=Tix.TOP, padx=3, pady=3)

###
### List of all the demos we want to show off
comments = {'widget' : 'Widget Demos', 'image' : 'Image Demos'}
samples = {'Balloon'		: 'Balloon',
	   'Button Box'		: 'BtnBox',
	   'Combo Box'		: 'ComboBox',
	   'Compound Image'	: 'CmpImg',
	   'Control'		: 'Control',
	   'Notebook'		: 'NoteBook',
	   'Option Menu'	: 'OptMenu',
	   'Popup Menu'		: 'PopMenu',
	   'ScrolledHList (1)'	: 'SHList1',
	   'ScrolledHList (2)'	: 'SHList2',
	   'Tree (dynamic)'	: 'Tree'
}

stypes = {}
stypes['widget'] = ['Balloon', 'Button Box', 'Combo Box', 'Control',
		    'Notebook', 'Option Menu', 'Popup Menu',
		    'ScrolledHList (1)', 'ScrolledHList (2)', 'Tree (dynamic)']
stypes['image'] = ['Compound Image']

def MkSample(nb, name):
    w = nb.page(name)
    prefix = Tix.OptionName(w)
    if not prefix:
	prefix = ''
    w.option_add('*' + prefix + '*TixLabelFrame*label.padX', 4)

    lab = Tix.Label(w, text='Select a sample program:', anchor=Tix.W)
    lab1 = Tix.Label(w, text='Source:', anchor=Tix.W)

    slb = Tix.ScrolledHList(w, options='listbox.exportSelection 0')
    slb.hlist['command'] = lambda args=0, w=w,slb=slb: Sample_Action(w, slb, 'run')
    slb.hlist['browsecmd'] = lambda args=0, w=w,slb=slb: Sample_Action(w, slb, 'browse')

    stext = Tix.ScrolledText(w, name='stext')
    stext.text.bind('<1>', stext.text.focus())
    stext.text.bind('<Up>', lambda w=stext.text: w.yview(scroll='-1 unit'))
    stext.text.bind('<Down>', lambda w=stext.text: w.yview(scroll='1 unit'))
    stext.text.bind('<Left>', lambda w=stext.text: w.xview(scroll='-1 unit'))
    stext.text.bind('<Right>', lambda w=stext.text: w.xview(scroll='1 unit'))

    run = Tix.Button(w, text='Run ...', name='run', command=lambda args=0, w=w,slb=slb: Sample_Action(w, slb, 'run'))
    view = Tix.Button(w, text='View Source ...', name='view', command=lambda args=0,w=w,slb=slb: Sample_Action(w, slb, 'view'))

    lab.form(top=0, left=0, right='&'+str(slb))
    slb.form(left=0, top=lab, bottom=-4)
    lab1.form(left='&'+str(stext), top=0, right='&'+str(stext), bottom=stext)
    run.form(left=str(slb)+' 30', bottom=-4)
    view.form(left=run, bottom=-4)
    stext.form(bottom=str(run)+' -5', left='&'+str(run), right='-0', top='&'+str(slb))

    stext.text['bg'] = slb.hlist['bg']
    stext.text['state'] = 'disabled'
    stext.text['wrap'] = 'none'
    #XXX    stext.text['font'] = fixed_font

    slb.hlist['separator'] = '.'
    slb.hlist['width'] = 25
    slb.hlist['drawbranch'] = 0
    slb.hlist['indent'] = 10
    slb.hlist['wideselect'] = 1

    for type in ['widget', 'image']:
	if type != 'widget':
	    x = Tix.Frame(slb.hlist, bd=2, height=2, width=150,
			  relief=Tix.SUNKEN, bg=slb.hlist['bg'])
	    slb.hlist.add_child(itemtype=Tix.WINDOW, window=x, state='disabled')
	x = slb.hlist.add_child(itemtype=Tix.TEXT, state='disabled',
				text=comments[type])
	for key in stypes[type]:
	    slb.hlist.add_child(x, itemtype=Tix.TEXT, data=key,
				text=key)
    slb.hlist.selection_clear()

    run['state'] = 'disabled'
    view['state'] = 'disabled'

def Sample_Action(w, slb, action):
    global demo

    run = w._nametowidget(str(w) + '.run')
    view = w._nametowidget(str(w) + '.view')
    stext = w._nametowidget(str(w) + '.stext')

    hlist = slb.hlist
    anchor = hlist.info_anchor()
    if not anchor:
	run['state'] = 'disabled'
	view['state'] = 'disabled'
    elif not hlist.info_parent(anchor):
	# a comment
	return

    run['state'] = 'normal'
    view['state'] = 'normal'
    key = hlist.info_data(anchor)
    title = key
    prog = samples[key]

    if action == 'run':
	exec('import ' + prog)
	w = Tix.Toplevel()
	w.title(title)
	rtn = eval(prog + '.RunSample')
	rtn(w)
    elif action == 'view':
	w = Tix.Toplevel()
	w.title('Source view: ' + title)
	LoadFile(w, demo.dir + '/samples/' + prog + '.py')
    elif action == 'browse':
	ReadFile(stext.text, demo.dir + '/samples/' + prog + '.py')

def LoadFile(w, fname):
    b = Tix.Button(w, text='Close', command=w.destroy)
    t = Tix.ScrolledText(w)
    #    b.form(left=0, bottom=0, padx=4, pady=4)
    #    t.form(left=0, bottom=b, right='-0', top=0)
    t.pack()
    b.pack()

    t.text['highlightcolor'] = t['bg']
    t.text['bd'] = 2
    t.text['bg'] = t['bg']
    t.text['wrap'] = 'none'

    ReadFile(t.text, fname)

def ReadFile(w, fname):
    old_state = w['state']
    w['state'] = 'normal'
    w.delete('0.0', Tix.END)

    try:
	f = open(fname)
	lines = f.readlines()
	for s in lines:
	    w.insert(Tix.END, s)
	f.close()
    finally:
#	w.see('1.0')
	w['state'] = old_state

if __name__ == '__main__':
    main()
