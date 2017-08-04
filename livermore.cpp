#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sqlite3.h>

#include "livermore.h"

using namespace std;


char*   g_sqlite_db = NULL; 
int g_stock_id = 0;

char g_record[MAX_FILE_PATH_LEN];

// Livemore Parameters

int StateNo=1;
bool InUpTrend= true;
bool InDnTrend= false;

int TrendSwitch=0;  /* 0: No-switch , 1: switch-to-UP , 2: switch-to-DN */

float UpTrend=0;
float NatRally=0;
float SecRally=0;
float SecReact=0;
float NatReact=0;
float DnTrend=0;

bool NatRallyReset=true;
bool NatReactReset=true;

float UpTrendPP=0;   // UTKP 
float NatReactPP=0;  // UBKP (Up Trend KeyPoint)
float NatRallyPP=0;  // DTKP (Dn Trend KeyPoint)
float DnTrendPP=0;   // DBKP

float ThreshholdPct = 0.1 ; 
float last_price=0;
int g_calc_duration = 5;

char g_calcDate[10] = {'\0'};
char UpTrendPP_Date[10] = {'\0'};
char NatReactPP_Date[10] = {'\0'};
char NatRallyPP_Date[10] = {'\0'};
char DnTrendPP_Date[10] = {'\0'};

float UpTrendPP_keep1=0;   // UTKP 
float NatReactPP_keep1=0;  // UBKP 
float NatRallyPP_keep1=0;  // DTKP 
float DnTrendPP_keep1=0;   // DBKP

void livermore_reset_params()
{
    if( InUpTrend && ( NatReact < (UpTrend /(1 + 1.5 * ThreshholdPct ))))
    {
        NatReactReset = true;
    }
    else if ( InDnTrend && ( NatRally > (DnTrend * ( 1 + 1.5 * ThreshholdPct))))
    {
        NatRallyReset = true;
    }
    if(TrendSwitch == 1 ){  /* Switch to Up Trend */
        cout << "Trend switch from Down to Up" << endl;
        UpTrendPP_keep1 = UpTrendPP;
        NatReactPP_keep1 = NatReactPP;
        //UpTrendPP = 0 ;
        //NatReactPP = 0 ;

        //UpTrend=0;
        //NatReact=0;
        //SecReact=0;
        NatRally=0;
        SecRally=0;
        DnTrend=0;

        memset(UpTrendPP_Date,'\0',sizeof(UpTrendPP_Date));
        memset(NatReactPP_Date,'\0',sizeof(NatReactPP_Date));
        TrendSwitch = 0;
    }
    if(TrendSwitch == 2 ){  /* Switch to Dn Trend */
        cout << "Trend switch from Up to Down" << endl;
        DnTrendPP_keep1 = DnTrendPP;
        NatRallyPP_keep1 = NatRallyPP; 
        //DnTrendPP = 0 ;
        //NatRallyPP = 0 ;

        UpTrend=0;
        NatReact=0;
        SecReact=0;
        //NatRally=0;
        //SecRally=0;
        //DnTrend=0;

        memset(DnTrendPP_Date,'\0',sizeof(DnTrendPP_Date));
        memset(NatRallyPP_Date,'\0',sizeof(NatRallyPP_Date));
        TrendSwitch = 0;
    }
}

void process_state_1(float price)
{
    if(price >= last_price)
    {
        if(price > UpTrend)
        {
            StateNo=1; 
            InUpTrend = true;
            InDnTrend = false;
            UpTrend = price ; 
        }
    }
    else  // price < last_price 
    {
        if (InDnTrend &&  price < DnTrendPP) 
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price ;
            UpTrendPP = UpTrend ;
            safecpy(UpTrendPP_Date, g_calcDate);
        }
        else if ( InUpTrend && price < ( UpTrend / (1+ 1.5 * ThreshholdPct )))
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price ;
            UpTrendPP = UpTrend ;
            safecpy(UpTrendPP_Date, g_calcDate);
            TrendSwitch = 2; /* switch to Down Trend */
        }
        else
        {
            if ( price < ( UpTrend / (1+ ThreshholdPct))) 
            {
                if ( NatReactReset || ( price < NatReactPP) )
                {
                    StateNo = 5;    // InNatReact 
                    NatReactReset = false;
                    NatReact = price ;
                    UpTrendPP = UpTrend;
                    safecpy(UpTrendPP_Date, g_calcDate);
                }
                else
                {
                    StateNo = 4;   // InSecReact
                    SecReact = price ;
                    UpTrendPP = UpTrend; 
                    safecpy(UpTrendPP_Date, g_calcDate);
                }
            }
        }
    }
}


void process_state_2(float price) // State: InNatRally 
{
    if(price >= last_price)
    {
        if(InUpTrend && ( price > UpTrendPP))
        {
           StateNo=1; // InUpTrend
           InUpTrend = true;
           InDnTrend = false;
           UpTrend = price;
        }
        else if (InDnTrend && price > DnTrend * ( 1 + 1.5 * ThreshholdPct))
        {
           StateNo=1; // InUpTrend
           InUpTrend = true;
           InDnTrend = false;
           UpTrend = price;
           TrendSwitch = 1; /* switch to Up Trend */
        }
        else
        {
            if ( price > NatRally) 
            {
                StateNo=2 ; // InNatRally
                NatRally = price ;
            }
        }
    }
    else 
    {
        if( InDnTrend && (price < DnTrendPP)) 
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
            NatRallyPP = NatRally;
            safecpy(NatRallyPP_Date, g_calcDate);
        }
        else if(InUpTrend && price < ( UpTrend / ( 1+1.5*ThreshholdPct)))
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
            NatRallyPP = NatRally;
            safecpy(NatRallyPP_Date, g_calcDate);
            TrendSwitch = 2; /* switch to Down Trend */
        }
        else
        {
            if( price < ( NatRally / (1+ ThreshholdPct)))
            {
                if( NatReactReset ||  price < NatReactPP )
                {
                    StateNo=5; //InNatReact
                    NatReactReset= false;
                    NatReact = price ;
                    NatRallyPP = NatRally;
                    safecpy(NatRallyPP_Date, g_calcDate);
                }
                else
                {
                    StateNo=4; // InSecReact
                    SecReact = price;
                    NatRallyPP = NatRally;
                    safecpy(NatRallyPP_Date, g_calcDate);
                }
            }
        }
    }
}

void process_state_3(float price) // State: InSecRally 
{
    if(price >= last_price)
    {
        if(InUpTrend && ( price > UpTrendPP))
        {
           StateNo=1; // InUpTrend
           InUpTrend = true;
           InDnTrend = false;
           UpTrend = price;
        }
        else if(InDnTrend && price > DnTrend * ( 1 + 1.5 * ThreshholdPct))
        {
           StateNo=1; // InUpTrend
           InUpTrend = true;
           InDnTrend = false;
           UpTrend = price;
           TrendSwitch = 1; /* switch to Up Trend */
        }
        else
        {
            if ( price > NatRally) 
            {
                StateNo=2 ; // InNatRally
                NatRally = price ;
            }
            else if ( price > SecRally)
            {
                StateNo=3 ; // InSecRally
                SecRally = price ;
            }
        }
    }
    else 
    {
        if(InDnTrend && (price < DnTrendPP))
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
        }
        else if( InUpTrend && price < ( UpTrend / ( 1+1.5 * ThreshholdPct )))
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
            TrendSwitch = 2; /* switch to Down Trend */
        }
        else
        {
            if( price < ( SecRally / (1+ ThreshholdPct)))
            {
                if( price < NatReactPP )
                {
                    StateNo=5; //InNatReact
                    NatReact = price ;
                }
                else
                {
                    StateNo=4; // InSecReact
                    SecReact = price;
                }
            }
        }
    }
}
void process_state_4(float price) // State: InSecReact
{
    if(price >= last_price)
    {
        if( InUpTrend && ( price > UpTrendPP))
        {
            StateNo=1; // InUpTrend
            InUpTrend = true;
            InDnTrend = false;
            UpTrend = price;
        }
        else if(InDnTrend && price > DnTrend * ( 1 + 1.5 * ThreshholdPct))
        {
            StateNo=1; // InUpTrend
            InUpTrend = true;
            InDnTrend = false;
            UpTrend = price;
            TrendSwitch = 1; /* switch to Up Trend */
        }
        else
        {
            if ( price > SecReact * ( 1+ ThreshholdPct))
            {
                if ( price > NatRally) 
                {
                    StateNo=2 ; // InNatRally
                    NatRally = price ;
                }
                else
                {
                    StateNo=3 ; // InSecRally
                    SecRally = price ;
                }
            }
        }
    }
    else 
    {
        if( InDnTrend && (price < DnTrendPP)) 
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
        }
        else if(InUpTrend && price < ( UpTrend / ( 1+ 1.5 *ThreshholdPct)))
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
            TrendSwitch = 2; /* switch to Down Trend */
        }
        else
        {
            if( price < NatReact)
            {
                StateNo=5; //InNatReact
                NatReact = price ;
            }
            else
            {
                if( price < SecReact)
                {
                    StateNo=4; //InNatReact
                    SecReact = price ;
                }
            }
        }
    }
}
void process_state_5(float price) // State: InNatReact
{
    if(price >= last_price)
    {
        if( InUpTrend && ( price > UpTrendPP))
        {
            StateNo=1; // InUpTrend
            InUpTrend = true;
            InDnTrend = false;
            UpTrend = price;
            NatReactPP = NatReact;
            safecpy(NatReactPP_Date, g_calcDate);
        }
        else if(InDnTrend && price > DnTrend * ( 1 + 1.5 * ThreshholdPct))
        {
            StateNo=1; // InUpTrend
            InUpTrend = true;
            InDnTrend = false;
            UpTrend = price;
            NatReactPP = NatReact;
            safecpy(NatReactPP_Date, g_calcDate);
            TrendSwitch = 1; /* switch to Up Trend */
        }
        else
        {
            if ( price > NatReact * ( 1+ ThreshholdPct ))
            {
                if (NatRallyReset ||  price > NatRallyPP) 
                {
                    StateNo=2 ; // InNatRally
                    NatRallyReset = false;
                    NatRally = price ;
                    NatReactPP = NatReact;
                    safecpy(NatReactPP_Date, g_calcDate);
                }
                else
                {
                    StateNo=3 ; // InSecRally
                    SecRally = price ;
                    NatReactPP = NatReact;
                    safecpy(NatReactPP_Date, g_calcDate);
                }
            }
        }
    }
    else 
    {
        if( InDnTrend && (price < DnTrendPP))
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
        }
        else if(InUpTrend && price < ( UpTrend / ( 1+1.5*ThreshholdPct)))
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
            TrendSwitch = 2; /* switch to Down Trend */
        }
        else
        {
            if( price < NatReact)
            {
                StateNo=5; //InNatReact
                NatReact = price ;
            }
        }
    }
}
void process_state_6(float price) // State: InDnTrend
{
    if(price >= last_price)
    {
        if( InUpTrend && ( price > UpTrendPP))
        {
            StateNo=1; // InUpTrend
            InUpTrend = true;
            InDnTrend = false;
            UpTrend = price;
            DnTrendPP = DnTrend;
            safecpy(DnTrendPP_Date, g_calcDate);
        }
        else if(InDnTrend && price > DnTrend * ( 1 + 1.5 * ThreshholdPct))
        {
            StateNo=1; // InUpTrend
            InUpTrend = true;
            InDnTrend = false;
            UpTrend = price;
            DnTrendPP = DnTrend;
            safecpy(DnTrendPP_Date, g_calcDate);
            TrendSwitch = 1; /* switch to Up Trend */
        }
        else
        {
            if ( price > DnTrend * ( 1+ ThreshholdPct ))
            {
                if (NatRallyReset ||  price > NatRallyPP) 
                {
                    StateNo=2 ; // InNatRally
                    NatRallyReset = false;
                    NatRally = price ;
                    DnTrendPP = DnTrend;
                    safecpy(DnTrendPP_Date, g_calcDate);
                }
                else
                {
                    StateNo=3 ; // InSecRally
                    SecRally = price ;
                    DnTrendPP = DnTrend;
                    safecpy(DnTrendPP_Date, g_calcDate);
                }
            }
        }
    }
    else 
    {
        if( price < DnTrend)
        {
            StateNo = 6;  // InDnTrend
            InDnTrend = true;
            InUpTrend = false;
            DnTrend = price;
        }
    }
}
void livermore(float price , char* date __attribute__((unused)) )
{
    switch(StateNo){
    case 1:
        process_state_1(price);
        break;
    case 2:
        process_state_2(price);
        break;
    case 3:
        process_state_3(price);
        break;
    case 4:
        process_state_4(price);
        break;
    case 5:
        process_state_5(price);
        break;
    case 6:
        process_state_6(price);
        break;
    }
    livermore_reset_params();
    last_price = price ; 
}


void print_usage()
{
    printf( "Build Date:%s %s\n", __DATE__ , __TIME__);
    printf( "Usage: livermore [options] <sqlite db file> < g_stock_id> \n");
    printf( "Options:\n" );
    printf( "   -o: output_csv_file_path.\n" );
    printf( "   -t: Threshold Percentage:  10 (pct, default).\n" );
    printf( "   -d: Calulate Duration: 5 (pick data every 5 days, default).\n" );
    exit(1);
}

void parse_command_line(int argc, char** argv)
{
    int ch;
    cout << "Parsing command line arguments .. " << endl;

    while ((ch = getopt(argc, argv, "o:t:d:")) != -1) {
        switch (ch) {
        case 'd':
        {
            g_calc_duration = atoi (optarg);
            break;
        }
        case 'o':
        {
            safecpy(g_record, optarg);
	    printf("g_record= %s\n" , g_record);
            break;
        }
        case 't':
        {
            ThreshholdPct = (float ) atoi(optarg);
           
	    printf("ThreshholdPct = %.2f\n" , ThreshholdPct);
            break;
        }
	case 'h':
        default:
            print_usage();
            break;
        }
    }



    if (argc != 2 + optind) {
        printf("Insufficient argument provided!\n");
        print_usage();    
    }
    g_sqlite_db = argv[optind];
    g_stock_id = atoi(argv[optind+1]);
}

static int callback(void *data __attribute__((unused)) , int argc   __attribute__((unused)) , char **argv, char **azColName  __attribute__((unused)) ){

   float price = 0 ;
   //char date[10] = {'\0'};

   price = atof(argv[0]);
   safecpy ( g_calcDate , argv[1]);
   /*
   int i;
   for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   */
   //printf("%d | %f | %s \n", g_stock_id, price , date);
   livermore( price , g_calcDate ); 

   return 0;
}

void show_result()
{
    char mkt_keys[MAX_FILE_PATH_LEN];
    char buyin[2] = "N";
    char sellout[2] = "N";
    printf("Market Key for %d :: \n" , g_stock_id );
    printf("---- StateNo : %d \n" , StateNo);

    printf("----  UpTrend  Key : %.2f \n" ,UpTrend);
    printf("----  NatReact Key : %.2f \n" ,NatReact);
    printf("----  SecReact Key : %.2f \n" ,SecReact);
    printf("----  NatRally Key : %.2f \n" ,NatRally);
    printf("----  SecRally Key : %.2f \n" ,SecRally);
    printf("----  DnTrend  Key : %.2f \n" ,DnTrend);
  
    printf("--------  UpTrendPP  : %.2f \n" , UpTrendPP);
    printf("--------  NatReactPP : %.2f \n" , NatReactPP);
    printf("--------  NatRallyPP : %.2f \n" , NatRallyPP);
    printf("--------  DnTrendPP  : %.2f \n" , DnTrendPP);

    if (InUpTrend)
    {
        if(InUpTrend && last_price > UpTrendPP){
            printf(" ******** BUY-IN SIGNAL for %d ***, now price is %.2f , UpTrendPP  is %.2f\n" , g_stock_id , last_price, UpTrendPP);
            safecpy( buyin, "Y");
        }
        else if( NatReactPP != 0 && last_price < NatReactPP / (1 + ThreshholdPct / 2 ))
        {
            printf(" ******** SELL-OUT SIGNAL for %d ***, now price is %.2f , NatReactPP is %.2f\n" , g_stock_id , last_price, NatReactPP);
            safecpy( sellout, "Y");
        }
        printf("----   Trend : Up \n");
        sprintf(mkt_keys,"%d,%s,%s,%d,UP,%.2f,%s,%.2f,%s,%.2f,%s,%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f",
           g_stock_id, buyin, sellout, StateNo , UpTrendPP,UpTrendPP_Date, NatReactPP, NatReactPP_Date,NatRallyPP,NatRallyPP_Date, DnTrendPP,DnTrendPP_Date,UpTrendPP_keep1, NatReactPP_keep1, NatRallyPP_keep1, DnTrendPP_keep1,last_price); 

    }
    if (InDnTrend)
    {
        if (NatRallyPP != 0 && last_price > NatRallyPP * (1 + ThreshholdPct / 2 ))
        {
            printf(" ******** BUY-IN SIGNAL for %d ***, now price is %.2f , NatRallyPP is %.2f\n" , g_stock_id , last_price, NatRallyPP);
            safecpy( buyin, "Y");
        }
        else if ( InDnTrend && last_price < DnTrendPP )
        {
            printf(" ******** SELL-OUT SIGNAL for %d ***, now price is %.2f , DnTrendPP is %.2f\n" , g_stock_id , last_price, DnTrendPP);
            safecpy( sellout, "Y");
        }
        printf("----   Trend : Down \n");
        sprintf(mkt_keys,"%d,%s,%s,%d,DN,%.2f,%s,%.2f,%s,%.2f,%s,%.2f,%s,%.2f,%.2f,%.2f,%.2f,%.2f",
           g_stock_id, buyin, sellout, StateNo , UpTrendPP,UpTrendPP_Date, NatReactPP, NatReactPP_Date,NatRallyPP,NatRallyPP_Date, DnTrendPP,DnTrendPP_Date,UpTrendPP_keep1, NatReactPP_keep1, NatRallyPP_keep1, DnTrendPP_keep1,last_price); 

    }

    ofstream records(g_record, ios::out | ios::app);
    records << mkt_keys << '\n';
    records.flush();
}

int main(int argc, char **argv)
{
    parse_command_line(argc, argv);   
    sqlite3 *db;
    const char* data = "Callback function called";
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open( g_sqlite_db , &db);
    if( rc ){
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return(0);
    }else{
        fprintf(stderr, "Opened database successfully\n");
    }

    char sql[MAX_STR_TRANS_LEN];
    sprintf(sql, "create temporary table rotate as select price,date from day_price where id = %d order by date ;"
                 "select * from rotate where rowid %s %d = 0 or rowid = (select max(rowid) from rotate)", g_stock_id, "%", g_calc_duration ); 


    rc = sqlite3_exec(db, sql, callback, (void*)data, &zErrMsg);
    if( rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }
    else{
      fprintf(stdout, "Operation done successfully\n");
    }
    sqlite3_close(db);
    show_result();
    return 0;
}
