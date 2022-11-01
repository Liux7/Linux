set ts=4
set softtabstop=4
set shiftwidth=4
set autoindent
set nu
set rnu


 nmap <silent> <F9> :TagbarToggle<CR>        "按F9即可打开tagbar界面
 let g:tagbar_ctags_bin = 'ctags'                       "tagbar以来ctags插件
 let g:tagbar_left = 1                                          "让tagbar在页面左侧显示，默认右边
 let g:tagbar_width = 30                                     "设置tagbar的宽度为30列，默认40
 let g:tagbar_autofocus = 1                                "这是tagbar一打开，光标即在tagbar页面内，默认在vim打开的文件内
 let g:tagbar_sort = 0                                         "设置标签不排序，默认排序



au BufReadPost * if line("'\"") > 0|if line("'\"") <= line("$")|exe("norm '\"")|else|exe "norm $"|endif|endif

call plug#begin()
Plug 'preservim/tagbar'
Plug 'preservim/NERDTree'
Plug 'jiangmiao/auto-pairs'
Plug 'vim-airline/vim-airline'
Plug 'vim-airline/vim-airline-themes'
call plug#end()
