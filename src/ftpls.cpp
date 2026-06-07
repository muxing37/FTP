#include "ftpls.h"

struct option {
    int a;
    int l;
};
struct file {
    char *path;
    char *rpath;
    char *name;
    struct stat *im;
    int statp;
};
struct option op={0,0};
int ex=0;
size_t ljc=0;
size_t ljca=0;
int getlen(long long n) {
    if(n==0) return 1;
    int i;
    for(i=0;n>0;i++) {
        n=n/10;
    }
    return i;
}

int lujingzhengxu(const void *a,const void *b) {
    const struct file *in1=(const struct file *)a;
    const struct file *in2=(const struct file *)b;
    setlocale(LC_COLLATE, "");
    return strcoll(in1->path,in2->path);
}

int comparzhengxu(const void *a,const void *b) {
    const struct file *in1=(const struct file *)a;
    const struct file *in2=(const struct file *)b;
    setlocale(LC_COLLATE, "");
    return strcoll(in1->name,in2->name);
}

char* getkey(struct file info) {
    struct stat st,lst;
    char *s=(char*)malloc(8*sizeof(char));
    if(lstat(info.rpath,&lst)<0) {
        strcpy(s,"none");
        return s;
    } if(S_ISLNK(lst.st_mode)) {
        if(stat(info.rpath,&st)<0) {
            strcpy(s,"or");
            return s;
        } else {
            strcpy(s,"ln");
            return s;
        }
    }
    st=lst;
    if(S_ISDIR(st.st_mode)) {
        if((st.st_mode&S_ISVTX) && (st.st_mode&S_IWOTH)) {
            strcpy(s,"tw");
            return s;
        } if(st.st_mode & S_IWOTH) {
            strcpy(s,"ow");
            return s;
        }
        strcpy(s,"di");
        return s;
    } if(S_ISFIFO(st.st_mode)) {
        strcpy(s,"pi");
        return s;
    } if(S_ISSOCK(st.st_mode)) {
        strcpy(s,"so");
        return s;
    } if(S_ISBLK(st.st_mode)) {
        strcpy(s,"bd");
        return s;
    } if(S_ISCHR(st.st_mode))  {
        strcpy(s,"cd");
        return s;
    } if(st.st_mode & S_ISUID) {
        strcpy(s,"su");
        return s;
    } if(st.st_mode & S_ISGID) {
        strcpy(s,"sg");
        return s;
    } if(st.st_mode & S_ISVTX) {
        strcpy(s,"st");
        return s;
    } if(st.st_mode & S_IXUSR) {
        strcpy(s,"ex");
        return s;
    } if(info.name[strlen(info.name)-1]=='#') {
        strcpy(s,"*#");
        return s;
    } if(info.name[strlen(info.name)-1]=='~') {
        strcpy(s,"*~");
        return s;
    }
    int i;
    for(i=strlen(info.name)-1;i>=0;i--) {
        if(info.name[i]=='.') {
            char *t=(char*)malloc(sizeof(char)*(strlen(info.name)-i+2));
            int j;
            t[0]='*';
            for(j=1;i<strlen(info.name);j++,i++) {
                t[j]=info.name[i];
            }
            t[j]='\0';
            free(s);
            return t;
        }
    }
    strcpy(s,"none");
    return s;
}
char* getcolor(struct file info) {
    char *key=getkey(info);
    const char *env=getenv("LS_COLORS");
    if(!env) return NULL;
    char *copy=strdup(env);
    if(!copy) return NULL;
    char *found=NULL;
    char *saveptr=NULL;
    for (char *p=strtok_r(copy,":",&saveptr);p;p=strtok_r(NULL,":",&saveptr)) {
        char *eq=strchr(p,'=');
        if(!eq) continue;
        *eq='\0';
        char *k=p;
        char *v=eq+1;
        if(strcmp(k,key)==0) {
            free(found);
            found=strdup(v);
        }
    }
    free(key);
    free(copy);
    return found;
}
int getwide(const char *str) {
    setlocale(LC_ALL, "");
    size_t len = strlen(str);
    wchar_t wstr[len+1];
    size_t wlen=mbstowcs(wstr,str,len+1);
    if(wlen==(size_t)-1) {
        return strlen(str);
    }
    int twide=0;
    int i;
    for(i=0;i<wlen;i++) {
        int wid=wcwidth(wstr[i]);
        if(wid<0) {
            wid=1;
        }
        twide=twide+wid;
    }
    return twide;
}
std::string printc(int qu,int q,struct file info) {
    int j=0;
    int kg=0;
    std::string result;
    char *sgr=getcolor(info);
    if(sgr!=NULL) {
        result += "\033[";
        result += sgr;
        result += "m";
        if (qu == 2 && q == 0) result += ' ';
        if (qu == 2 && q == 1) result += '\'';
        if (qu == 2 && q == 2) result += '"';
        result += info.name;
        if (qu == 2 && q == 0) result += ' ';
        if (qu == 2 && q == 1) result += '\'';
        if (qu == 2 && q == 2) result += '"';
        result += "\033[0m";
    } else {
        if (qu == 2 && q == 0) result += ' ';
        if (qu == 2 && q == 1) result += '\'';
        if (qu == 2 && q == 2) result += '"';
        result += info.name;
        if (qu == 2 && q == 0) result += ' ';
        if (qu == 2 && q == 1) result += '\'';
        if (qu == 2 && q == 2) result += '"';
    }
    free(sgr);
    return result;
}

std::vector<std::string> ls(int z,struct file pain);
int ifqute(char *name) {
    int i;
    for(i=0;i<strlen(name);i++) {
        if(name[i]==32 || name[i]==34 || name[i]==36 || name[i]==42 || name[i]==92 || 
           name[i]==40 || name[i]==41 || name[i]==33 || name[i]==94 || name[i]==38 ||
           name[i]==61 || name[i]==59 || name[i]==60 || name[i]==62 || name[i]==124 ||
           name[i]==63 || name[i]==96) return 1;
        if(name[i]==39) return 2;
    }
    return 0;
}
std::vector<std::string> output(int z,int filec,struct file pain,struct file *info) {
    int i,j;
    qsort(info,filec,sizeof(struct file),comparzhengxu);
    std::vector<std::string> res;
    if(z>1 && z<=ljc) printf("\n");
    int q=ifqute(pain.path);
    if((ljca>1 && z!=-1) && q==0) printf("%s:\n",pain.path);
    if((ljca>1 && z!=-1) && q==1) printf("'%s':\n",pain.path);
    if((ljca>1 && z!=-1) && q==2) printf("\"%s\":\n",pain.path);
    int l=0;
    int mino=0;
    int mblocks=0;
    int muid=0;
    int mgid=0;
    int mnlink=0;
    int msize=0;
    long long sum=0;
    int qu=0;
    for(i=0;i<filec;i++) {
        if((op.l==1) && info[i].statp<0) return res;

        if((op.l==1) && info[i].im->st_blocks!=1) info[i].im->st_blocks=info[i].im->st_blocks/2;
        if(ifqute(info[i].name)) {
            qu=2;
        }
        if((op.l==1) && z!=-1) {
            sum=sum+info[i].im->st_blocks;
        }

        if(op.l==1) {
            l=getlen(info[i].im->st_size);
            if(l>msize) msize=l;
            l=getlen(info[i].im->st_nlink);
            if(l>mnlink) mnlink=l;
            struct group *gr=getgrgid(info[i].im->st_gid);
            if(gr!=NULL) {
                l=strlen(gr->gr_name);
            } else {
                l=getlen(info[i].im->st_gid);
            }
            if(l>mgid) mgid=l;
            struct passwd *pw=getpwuid(info[i].im->st_uid);
            if(pw!=NULL) {
                l=strlen(pw->pw_name);
            } else {
                l=getlen(info[i].im->st_uid);
            }
            if(l>muid) muid=l;
        }
    }
    if(op.l==1) {
        int i,j;
        for(i=0;i<filec;i++) {
            std::ostringstream oss;

            char l1[11]="----------";
            if(S_ISDIR(info[i].im->st_mode)) l1[0]='d';
            if(S_ISLNK(info[i].im->st_mode)) l1[0]='l';
            if(info[i].im->st_mode &S_IRUSR) l1[1]='r';
            if(info[i].im->st_mode &S_IWUSR) l1[2]='w';
            if(info[i].im->st_mode &S_IXUSR) l1[3]='x';
            if(info[i].im->st_mode &S_IRGRP) l1[4]='r';
            if(info[i].im->st_mode &S_IWGRP) l1[5]='w';
            if(info[i].im->st_mode &S_IXGRP) l1[6]='x';
            if(info[i].im->st_mode &S_IROTH) l1[7]='r';
            if(info[i].im->st_mode &S_IWOTH) l1[8]='w';
            if(info[i].im->st_mode &S_IXOTH) l1[9]='x';

            oss << l1 << " ";
            oss << std::setw(mnlink) << info[i].im->st_nlink << " ";
            struct passwd *pw=getpwuid(info[i].im->st_uid);
            if(pw!=NULL) {
                oss << std::left << std::setw(muid) << pw->pw_name << " ";
            } else {
                oss << std::left << std::setw(muid) << info[i].im->st_uid << " ";
            }

            struct group  *gr=getgrgid(info[i].im->st_gid);
            if(gr!=NULL) {
                oss << std::left << std::setw(mgid) << gr->gr_name << " ";
            } else {
                oss << std::left << std::setw(mgid) << info[i].im->st_gid << " ";
            }
            oss << std::right << std::setw(msize) << info[i].im->st_size << " ";
            time_t now=time(NULL);
            double dif=difftime(now,info[i].im->st_mtime);
            char *loc=setlocale(LC_ALL,NULL);

            if(dif<6*30*24*60*60) {
                char yue[6];
                struct tm *tm=localtime(&info[i].im->st_mtime);
                strftime(yue,sizeof(yue),"%b",tm);

                if(loc[0]=='z' && loc[1]=='h') {
                    oss << std::setw(5) << yue << " ";
                } else {
                    oss << std::setw(3) << yue << " ";
                }
                oss << std::setw(2) << tm->tm_mday << " ";
                char s[6];
                strftime(s,sizeof(s),"%H:%M",tm);
                oss << s << " ";
            } else {
                char yue[6];
                struct tm *tm=localtime(&info[i].im->st_mtime);
                strftime(yue,sizeof(yue),"%b",tm);

                if(loc[0]=='z' && loc[1]=='h') {
                    oss << std::setw(5) << yue << " ";
                } else {
                    oss << std::setw(3) << yue << " ";
                }
                oss << std::setw(2) << tm->tm_mday << " ";
                char s[6];
                strftime(s,sizeof(s),"%Y",tm);
                oss << std::setw(5) << s << " ";
            }
            std::string temp = printc(qu,ifqute(info[i].name),info[i]);
            oss << temp;

            if(S_ISLNK(info[i].im->st_mode)) {
                char t[PATH_MAX + 1];
                int len;
                len=readlink(info[i].path,t,PATH_MAX);
                if(len>=0) {
                    t[len]='\0';
                    oss << " -> " << t;
                }
            }
            res.push_back(oss.str());
        }
    }
    return res;
}

int getinfo(int i,struct dirent *temp,struct file pain,struct file *info) {
    if(temp->d_name[0]=='.' && op.a==0) return 0;
    info[i].name=(char *)malloc(strlen(temp->d_name)+2);
    strcpy(info[i].name,temp->d_name);

    info[i].im=(struct stat *)malloc(sizeof(struct stat));

    char *t=(char*)malloc(sizeof(char)*(strlen(pain.rpath)+strlen(info[i].name)+3));
    strcpy(t,pain.rpath);
    if(pain.rpath[strlen(pain.rpath)-1]!='/') {
        t[strlen(pain.rpath)]='/';
        t[strlen(pain.rpath)+1]='\0';
    }
    strcat(t,info[i].name);
    info[i].rpath=(char*)malloc((strlen(t)+2)*sizeof(char));
    strcpy(info[i].rpath,t);
    lstat(info[i].rpath,info[i].im);
    struct stat st;

    info[i].statp=stat(info[i].rpath,&st);

    char *te=(char*)malloc(sizeof(char)*(strlen(pain.path)+strlen(info[i].name)+3));
    strcpy(te,pain.path);
    if(pain.path[strlen(pain.path)-1]!='/') {
        te[strlen(pain.path)]='/';
        te[strlen(pain.path)+1]='\0';
    }
    strcat(te,info[i].name);
    info[i].path=(char*)malloc((strlen(te)+2)*sizeof(char));
    strcpy(info[i].path,te);
    free(te);
    i++;
    free(t);
    return 1;
}

size_t grow(size_t c,size_t a,struct file **g) {
    if(c>a-2 || a==0) {
        if(a==0) {
            a=2;
            *g=(struct file *)malloc(sizeof(struct file)*a);
        } else {
            *g=(struct file *)realloc(*g,sizeof(struct file)*a*2);
            a=a*2;
        }
    }
    return a;
}
std::vector<std::string> firstls(size_t fc,struct file *first) {
    int i;
    std::vector<std::string> res;
    struct file *info=(struct file *)malloc(4*sizeof(struct file));
    size_t filec=0;
    size_t filea=0;
    filea=4;
    for(i=0;i<fc;i++) {
        filea=grow(filec,filea,&info);
        info[i].im=(struct stat*)malloc(sizeof(struct stat));
        info[i].path=(char*)malloc((strlen(first[i].path)+2)*sizeof(char));
        strcpy(info[i].path,first[i].path);
        info[i].name=(char*)malloc((strlen(first[i].path)+2)*sizeof(char));
        strcpy(info[i].name,first[i].path);
        info[i].rpath=(char*)malloc((strlen(first[i].rpath)+2)*sizeof(char));
        strcpy(info[i].rpath,first[i].rpath);
        stat(info[i].rpath,info[i].im);
        filec++;
    }
    res=output(-1,filec,*first,info);

    for(i=0;i<filec;i++) {
        free(first[i].path);
        free(first[i].rpath);
        free(info[i].im);
        free(info[i].path);
        free(info[i].rpath);
        free(info[i].name);
    }
    free(info);
    if(ljc>0) putchar('\n');
    return res;
}

std::vector<std::string> startls(int argc,char **argv,std::string cwd) {
    std::vector<std::string> res;

    op={0,0};
    ex=0;
    ljc=0;
    ljca=0;

    int i,j;
    op.l=1;

    for(i=1;i<argc;i++) {
        if(argv[i][1]=='-' && argv[i][0]=='-') {
            printf("ls: 未识别的选项 \"%s\"\n",argv[i]);
            return res;
        }
        for(int j=1;argv[i][j]!='\0';j++) {
            if(argv[i][0]=='-') {
                if(argv[i][j]=='a') {
                    op.a=1;
                } else {
                    printf("ls: 无效的选项 -- %c\n",argv[i][j]);
                    return res;
                }
            }
        }
    }
    struct file *pain=(struct file *)malloc(4*sizeof(struct file));
    size_t lja=0;
    lja=4;
    char *t=realpath(argv[0],NULL);
    if(t==NULL) {
        t=(char*)malloc(strlen(argv[0])+3*sizeof(char));
        strcpy(t,argv[0]);
    }
    pain[0].path=(char*)calloc(strlen(argv[0])+1,sizeof(char));
    strcpy(pain[0].path,argv[0]);
    for(i=0,j=0;i<strlen(pain[0].path);i++) {
        if(pain[0].path[i]=='/') {
            j++;
        }
    }
    if(j==1 && pain[0].path[0]=='.') {
        pain[0].path[1]='\0';
    } else {
        for(i=0;j>0;i++) {
            if(pain[0].path[i]=='/') {
                j--;
            }
            if(j==0) {
                pain[0].path[i]='\0';
            }
        }
    }
    pain[0].rpath=(char*)calloc(strlen(t)+2,sizeof(char));
    strcpy(pain[0].rpath,t);
    for(i=0,j=0;i<strlen(t);i++) {
        if(t[i]=='/') {
            j++;
        }
    }
    if(j==1) {
        pain[0].rpath[0]='/';
        pain[0].rpath[1]='\0';
    } else {
        for(i=0;j>0;i++) {
            if(pain[0].rpath[i]=='/') {
                j--;
            }
            if(j==0) {
                pain[0].rpath[i]='\0';
            }
        }
    }
    free(t);
    setlocale(LC_ALL, "");
    struct file *first=NULL;
    size_t fc=0;
    size_t fa=0;
    for(i=1;i<argc;i++) {
        if(argv[i][0]!='-') {
            ljca++;
            lja=grow(ljc,lja,&pain);
            if(argv[i][0]=='/') {
                DIR *dp=opendir(argv[i]);
                if(dp==NULL && (errno==2 || errno==20)) {
                    if(errno==20) {
                        fa=grow(fc,fa,&first);
                        first[fc].path=(char *)malloc(sizeof(char)*(strlen(argv[i])+20));
                        first[fc].rpath=(char *)malloc(sizeof(char)*(strlen(argv[i])+20));
                        strcpy(first[fc].path,argv[i]);
                        strcpy(first[fc].rpath,argv[i]);
                        closedir(dp);
                        fc++;
                        continue;
                    } else {
                        printf("ls: 无法访问 '%s': %s\n",argv[i],strerror(errno));
                        ex=2;
                        closedir(dp);
                        continue;
                    }
                }
                ljc++;
                pain[ljc].path=(char *)malloc(sizeof(char)*(strlen(argv[i])+1));
                pain[ljc].rpath=(char *)malloc(sizeof(char)*(strlen(argv[i])+1));
                strcpy(pain[ljc].path,argv[i]);
                strcpy(pain[ljc].rpath,argv[i]);
                closedir(dp);
            } else if ((argv[i][0]=='.' && argv[i][1]=='/') || (argv[i][0]=='.' && argv[i][1]=='.' && argv[i][2]=='/')) {
                char *t=realpath(argv[i],NULL);
                if(t==NULL && errno==2) {
                        printf("ls: 无法访问 '%s': %s\n",argv[i],strerror(errno));
                        ex=2;
                        free(t);
                        continue;
                }
                DIR *dp=opendir(t);
                if(dp==NULL && errno==20) {
                    if(errno==20) {
                        fa=grow(fc,fa,&first);
                        first[fc].path=(char*)calloc(strlen(argv[i])+1,sizeof(char));
                        first[fc].rpath=(char*)calloc(strlen(t)+1,sizeof(char));
                        strcpy(first[fc].path,argv[i]);
                        strcpy(first[fc].rpath,t);
                        free(t);
                        fc++;
                        closedir(dp);
                        continue;
                    } else {
                        printf("ls: 无法访问 '%s': %s\n",argv[i],strerror(errno));
                        ex=2;
                        free(t);
                        closedir(dp);
                        continue;
                    }
                }
                ljc++;
                pain[ljc].path=(char*)calloc(strlen(argv[i])+1,sizeof(char));
                pain[ljc].rpath=(char*)calloc(strlen(t)+1,sizeof(char));
                strcpy(pain[ljc].path,argv[i]);
                strcpy(pain[ljc].rpath,t);
                closedir(dp);
                free(t);
            } else {
                char *t=(char *)malloc(sizeof(char)*(strlen(argv[i])+strlen(pain[0].rpath)+2));
                strcpy(t,pain[0].rpath);
                t[strlen(pain[0].rpath)]='/';
                t[strlen(pain[0].rpath)+1]='\0';
                strcat(t,argv[i]);
                DIR *dp=opendir(t);
                if(dp==NULL && (errno==2 || errno==20)) {
                    if(errno==20) {
                        fa=grow(fc,fa,&first);
                        first[fc].path=(char *)malloc(sizeof(char)*(strlen(argv[i])+2));
                        strcpy(first[fc].path,argv[i]);
                        first[fc].rpath=(char *)malloc(sizeof(char)*(strlen(argv[i])+strlen(pain[0].rpath)+2));
                        strcpy(first[fc].rpath,t);
                        free(t);
                        closedir(dp);
                        fc++;
                        continue;
                    } else {
                        printf("ls: 无法访问 '%s': %s\n",argv[i],strerror(errno));
                        ex=2;
                        free(t);
                        closedir(dp);
                        continue;
                    }
                }
                ljc++;
                pain[ljc].path=(char *)malloc(sizeof(char)*(strlen(argv[i])+2));
                strcpy(pain[ljc].path,argv[i]);
                pain[ljc].rpath=(char *)malloc(sizeof(char)*(strlen(argv[i])+strlen(pain[0].rpath)+2));
                strcpy(pain[ljc].rpath,t);
                
                free(t);
                closedir(dp);
            }
            continue;
        }
    }
    if(ljc>1) {
        for(i=1;i<=ljc;i++) {
            pain[i].im=(struct stat*)malloc(sizeof(struct stat));
            // printf()
            stat(pain[i].rpath,pain[i].im);
        }
        struct file tem[ljc];
        memcpy(tem,&pain[1],sizeof(struct file)*(ljc));
        qsort(tem,ljc,sizeof(struct file),lujingzhengxu);
        for (i=1;i<ljc+1;i++) {
            pain[i]=tem[i-1];
        }
    }

    if(first!=NULL) {
        res=firstls(fc,first);
    }
    if((first!=NULL && ljc>0) || first==NULL) {
        if(ljc==0 && ljca==0) {
            res=ls(i,pain[0]);
        } else {
            for(i=1;i<=ljc;i++) {
                res=ls(i,pain[i]);
            }
        }
    }
    if(ljc!=0) {
        free(pain[0].path);
        free(pain[0].rpath);
    }
    for(i=1;i<=ljc && ljc!=1;i++) {
        free(pain[i].im);
    }
    free(first);
    free(pain);
    return res;
}

std::vector<std::string> ls(int z,struct file pain) {
    int i,j;
    j=0;
    std::vector<std::string> res;
    struct file *info=(struct file *)malloc(32*sizeof(struct file));
    size_t filec=0;
    size_t filea=0;
    filea=32;
    DIR *dp=opendir(pain.rpath);
    struct stat st,lst;
    int stp=stat(pain.rpath,&st);
    int lstp=lstat(pain.rpath,&lst);
    if(dp==NULL || (stp<0 && lstp<0)) {
        printf("ls: 无法打开目录 '%s': %s\n",pain.path,strerror(errno));
        ex=2;
        free(info);
        if(z==-2) return res;
        free(pain.path);
        free(pain.rpath);
        return res;
    }
    struct dirent *temp;
    i=0;
    while((temp=readdir(dp))!=NULL ) {
        if(filec>=filea-2) {
            info=(struct file *)realloc(info,sizeof(struct file)*filea*2);
            filea=filea*2;
        }
        if(getinfo(filec,temp,pain,info)) filec++;
    }
    closedir(dp);

    res=output(z,filec,pain,info);

    for(i=0;i<filec;i++) {
        free(info[i].path);
        free(info[i].rpath);
        free(info[i].name);
        free(info[i].im);
    }
    if(z>=0) {
        free(pain.path);
        free(pain.rpath);
    }
    free(info);
    return res;
}
