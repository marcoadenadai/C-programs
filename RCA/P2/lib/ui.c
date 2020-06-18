#include "ui.h"

/////////////////////////////////////////////////////////////////////////////////////////////////

contato * draw_novocontato(WINDOW * w){
    char input[32];
    memset(input,'\0',32);
    draw_inputbox(w, "Novo Contato, insira o Nome", input, 32);
    if(strlen(input)<1)
        return NULL;
    contato * ret = (contato *) malloc(sizeof(contato));
    strcpy(ret->nome,input);
    memset(input,'\0',32);
    draw_inputbox(w, "Novo Contato, insira o Telefone", input, 12);
    if(strlen(input)<1){
        free(ret);
        return NULL;
    }
    strcpy(ret->telefone,input);
    return ret;
}

void draw_send_txt(WINDOW * w, char * dest, char * input){
    wclear(w);
    box(w,0,0);
    mvwprintw(w, 0, 1, "[Para: %s]", dest);
    curs_set(1);
    echo();
    nocbreak();
    mvwgetnstr(w,2,3,input,256);
    //FIX INPUT: ACEITANDO ^C CARACTERES ESPECIAIS, MOSTRAR MSGBOX WARNING
    cbreak();
    noecho();
    curs_set(0);
    wrefresh(w);
}

void draw_messenger(WINDOW * w, char lines[19][77]){
    wclear(w);
    box(w, 0, 0);
    int i,j;
    for(i=0;i<19;i++){
        for(j=0;j<77;j++)
            mvwprintw(w, i+1, j+1, "%c", lines[i][j]);
    }
    wrefresh(w);
}

byte advance_lines(char lines[19][77], int advance){//return new cursor pos
    byte i;
    for(i=0;i<19-advance;i++){
        memset(lines[i],' ',77);
        strcpy(lines[i],lines[i+advance]);
    }
    return i;
}

void print_recebida(char tel[13], Contatos * C, char lines[19][77], char * msg){
    byte jl = 1+(strlen(msg)/77)+1;
    if(strlen(msg)%77!=0)
        jl++;
    
    byte curso = advance_lines(lines,jl);

    char tmp_line[77];
    memset(tmp_line,' ',77);

    char * autor = BuscaNome(C,tel);
    if(!autor)
        sprintf(tmp_line,">> (%s) Enviou: ",tel);
    else
        sprintf(tmp_line,">> %s Enviou: ",autor);
    memset(lines[curso],' ',77);
    strcpy(lines[curso],tmp_line);
    curso++;

    byte i,j,x=0;
    for(i=0;i<strlen(msg)/77 ; i++){
        memset(lines[curso],' ',77);
        for(j=0;j<77;j++, x++)
            lines[curso][j]=msg[x];
        (curso)++;
    }
    memset(lines[curso],' ',77);
    char * hndl = msg + x;
    strcpy(tmp_line,hndl);
    strcpy(lines[curso],tmp_line);
}

void print_enviada(char tel[13], Contatos * C, char lines[19][77], char * msg){
    byte jl = 1+(strlen(msg)/77)+1;
    if(strlen(msg)%77!=0)
        jl++;
    
    byte curso = advance_lines(lines,jl);

    char tmp_line[77];
    memset(tmp_line,' ',77);
    byte strl = 0;
    char * hndl;
    char * autor = BuscaNome(C,tel);
    if(!autor){
        strl=strlen(tel);
        hndl = tmp_line + 66 - strl;
        sprintf(hndl,"Para: (%s) <<",tel);
    }
    else{
        strl=strlen(autor);
        hndl = tmp_line + 66 - strl;
        sprintf(hndl,"Para:  %s  <<",autor);
    }
        
    memset(lines[curso],' ',77);
    strcpy(lines[curso],tmp_line);
    curso++;

    byte i,j,x=0;
    for(i=0;i<strlen(msg)/77 ; i++){
        memset(lines[curso],' ',77);
        for(j=0;j<77;j++, x++)
            lines[curso][j]=msg[x];
        (curso)++;
    }
    memset(lines[curso],' ',77);
    hndl = msg + x;
    strcpy(tmp_line,hndl);
    strcpy(lines[curso],tmp_line);
}

FILE * draw_send_img(WINDOW * w, char * dest, int EXIT_KEY, char ** fname){//writes fname
    wclear(w);
    box(w,0,0);
    mvwprintw(w, 0, 1, "Enviar imagem [Para: %s]", dest);
    mvwprintw(w, 1, 1, "Selecione uma imagem da pasta que este modulo se encontra:");
    DIR * d = opendir(".");
    struct dirent * dir;
    char * ptr = NULL;
    char ** content = NULL;
    int i, c, option=0, x=2, y=3, count = 0;
    if(d){
        while((dir = readdir(d))){
            ptr = dir->d_name + (strlen(dir->d_name) - 4);
            if(!strcmp(ptr,".jpg") || !strcmp(ptr,".gif") || !strcmp(ptr,".bmp") || 
            !strcmp(ptr,".png") || !strcmp((ptr-1),".jpeg")){
                content = (char **) realloc(content, (count+1)*sizeof(char*));
                content[count] = (char *) malloc(strlen(dir->d_name)+1);
                strcpy(content[count],dir->d_name);
                count++;
            }
        }
        closedir(d);
    }
    while(1){
        for(i=0, y=4;i<count;i++, y++){
            if(option==i){
                wattron(w, A_REVERSE); 
                mvwprintw(w, y, x, "%s", content[i]);
                wattroff(w, A_REVERSE); 
            }
            else
                mvwprintw(w, y, x, "%s", content[i]);
        }
        c=wgetch(w);
        if(c == EXIT_KEY && EXIT_KEY >= 0){
            wrefresh(w);
            return NULL;
        }
        switch(c){
            case KEY_UP:
                if(option==0)
                    option=count-1;
                else
                    option--;
                break;
            case KEY_DOWN:
                if(option==count-1)
                    option=0;
                else
                    option++;
                break;
            case KEY_ENTERR:
                break;
        }
        if(c==KEY_ENTERR)
            break;
    }
    wrefresh(w);
    if(count<=0)
        return NULL;
    *fname = (char *) malloc( sizeof(char)* (strlen(content[option])+1) );
    strcpy(*fname,content[option]);
    FILE * fp = fopen(content[option],"rb");
    for(i=0;i<count;i++)
        free(content[i]);
        free(content);
    return fp;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//FUNCOES DIALOGS SIMPLES: (podem ser reutilizadas)
void draw_box(WINDOW * w){
    box(w, 0, 0);
    wrefresh(w);
}

void draw_inputbox(WINDOW * w, char * title, char * input, int size){
    wclear(w);
    box(w,0,0);
    mvwprintw(w, 0, 1, "<%s >", title);
    curs_set(1);
    echo();
    nocbreak();
    //mvwprintw(w,1,1,"                                    ");
    //mvwprintw(w,2,1,"                                    ");
    mvwgetnstr(w,2,3,input,size);
    //FIX INPUT: ACEITANDO ^C CARACTERES ESPECIAIS, MOSTRAR MSGBOX WARNING
    cbreak();
    noecho();
    curs_set(0);
    wrefresh(w);
}

bool draw_msgbox(WINDOW * w, char * title, char * txt){
    wclear(w);
    keypad(w, TRUE);
    box(w,0,0);
    mvwprintw(w, 0, 1, "<%s >", title);
    mvwprintw(w,1,3,"%s",txt);
    bool option=0;
    int c;
    while(1){
        switch(option){
            case 0:
                mvwprintw(w,4,18," OK  [CANCEL]");
                break;
            case 1:
                mvwprintw(w,4,18,"[OK]  CANCEL ");
                break;
        }
        c=wgetch(w);
        if(c==KEY_SPACE){
            if(option)
                option=0;
            else
                option=1;
        }
        else if(c==KEY_LEFT && option==0)
            option=1;
        else if(c==KEY_RIGHT && option==1)
            option=0;
        else if(c==KEY_ENTERR)
            break;
    }
    wrefresh(w);
    return option;
}

int draw_menu(WINDOW * w, char * title, char ** content, int n, int EXIT_KEY){
    wclear(w);
    keypad(w, TRUE);
    box(w,0,0);
    mvwprintw(w, 0, 1, "<%s >", title);
    int option = 0;
    int c, i, x=2, y=1;
    while(1){
        for(i=0, y=1;i<n;i++, y++){
            if(option==i){
                wattron(w, A_REVERSE); 
                mvwprintw(w, y, x, "%s", content[i]);
                wattroff(w, A_REVERSE); 
            }
            else
                mvwprintw(w, y, x, "%s", content[i]);
        }
        c=wgetch(w);
        if(c == EXIT_KEY && EXIT_KEY >= 0){
            wrefresh(w);
            return -1;
        }
        switch(c){
            case KEY_UP:
                if(option==0)
                    option=n-1;
                else
                    option--;
                break;
            case KEY_DOWN:
                if(option==n-1)
                    option=0;
                else
                    option++;
                break;
            case KEY_ENTERR:
                break;
        }
        if(c==KEY_ENTERR)
            break;
    }
    wrefresh(w);
    return option;
}

menu_options * draw_menu_multi(WINDOW * w, char * title, char ** content, int n, int EXIT_KEY){
    wclear(w);
    keypad(w, TRUE);
    box(w,0,0);
    mvwprintw(w, 0, 1, "<%s >", title);

    int option = 0;
    int c, i, x=2, y=1;
    
    bool select[n];//* select = (bool *) malloc(sizeof(bool)*n);
    for(i=0;i<n;i++)
        select[i]=false;

    while(1){
        for(i=0, y=1;i<n;i++, y++){
            if(option==i){
                wattron(w, A_REVERSE); 
                if(select[i])
                    mvwprintw(w, y, x, "[*] ", select[i]);
                else
                    mvwprintw(w, y, x, "[ ] ", select[i]);
                mvwprintw(w, y, x+4, "%s", content[i]);
                wattroff(w, A_REVERSE); 
            }
            else{
                if(select[i])
                    mvwprintw(w, y, x, "[*] ", select[i]);
                else
                    mvwprintw(w, y, x, "[ ] ", select[i]);
                mvwprintw(w, y, x+4, "%s", content[i]);
            }
                
        }
        c=wgetch(w);
         if(c == EXIT_KEY && EXIT_KEY >= 0){
            wrefresh(w);
            return NULL;
        }
        switch(c){
            case KEY_UP:
                if(option==0)
                    option=n-1;
                else
                    option--;
                break;
            case KEY_DOWN:
                if(option==n-1)
                    option=0;
                else
                    option++;
                break;
            case KEY_SPACE:
                if(select[option])
                    select[option]=false;
                else
                    select[option]=true;
                break;
            case KEY_ENTERR:
                break;
        }
        if(c==KEY_ENTERR)
            break;
    }
    wrefresh(w);
    int total=0;
    for(i=0;i<n;i++){
        if(select[i])
            total++;
    }

    menu_options * ret = (menu_options *) malloc(sizeof(menu_options));
    ret->total = total;
    unsigned short * slc = (unsigned short *) malloc(sizeof(unsigned short)*total);
    int j=0;
    for(i=0;i<n;i++){
        if(select[i]==true){
            slc[j]=i;
            j++;
        }
    }
    ret->selected_id = slc;
    return ret;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

/* ENTENDER MELHOR O WREFRESH ********************************
The routine wrefresh copies the named window to the
physical terminal screen, taking into account what is
already there to do optimizations.

If the programmer wishes to output several windows at once,
a series of calls to wrefresh results in alternating calls
to wnoutrefresh and doupdate, causing several bursts of 
output to the screen. By first calling wnoutrefresh for each
window, it is then possible to call doupdate once, resulting
in only one burst of output, with fewer total characters 
transmitted and less CPU time used. *************************/
