// 6AFS-Test.cpp
#define 	DEBUGSS	0

#include	<windows.h>
#include	<iostream>
#include	<stdio.h>
#include	<conio.h>
#include	<time.h>
#include	<cstring>

int SetComAttr(HANDLE fdc);

int main()
	{
	FILE		*fd;
	HANDLE		fdc;
	char		fname[64];
	char		devname[64];
	char		str[256];
	char		csv[] =".csv";
	unsigned short	data[6];
	int			comNo;
	int			tick;
	int			clk, clkb, clkb2, clk0;
	int			tw,ts;
	int			num;
	int			i,j;
	double		ndata[6];
	double		zdata[11][6];
	double		hdata[10][6]={};
	double		z[6];
	DWORD		n;

	fd = NULL;
	fdc = NULL;


	// COMポートをオープン
	printf("Enter COM port > ");
	scanf("%d", &comNo);
	printf("Open COM%d\n", comNo);

	sprintf(devname, "COM%d", comNo);
	fdc = CreateFile(devname, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
	if (fdc == INVALID_HANDLE_VALUE)
		goto over;
//start :
	// サンプリング周期を得る
	tw = 16;
	printf("Enter sampling interval (ms) > ");
	scanf("%d", &tw);
	printf("Sampling interval = %d ms\n", tw);

	//サンプリング時間の取得
	printf("Enter sampling rate (s) > ");
	scanf("%d", &ts);
	printf("Sampling rate = %d s\n", ts);

	// COMポートのボーレート等を設定
	SetComAttr(fdc);

	/*printf("Enter File name > ");
	scanf("%s", fname);
	strcat(fname,csv);
	fd = fopen(fname, "w");
	if (!fd)
		goto over;

	sprintf(str, "Time[ms],Fx[N],Fy[N],Fz[N],Mx[N・m],My[N・m],Mz[N・m]\n");
	fprintf(fd, str);

	// データを読み出す
	printf("=== record data ===\n");*/
	clk0 = clock() / (CLOCKS_PER_SEC / 1000);
	clkb = 0;
	clkb2 = 0;
	num = 0;

	// 単データリクエスト（初回分）
	WriteFile(fdc, "R", 1, &n, 0);

	printf("零点検出中\nセンサに触れないでください\n");
	for (j=0;j<=10;j++)
		{
		// サンプリング周期だけ待つ
		while (true)
			{
			clk = clock() / (CLOCKS_PER_SEC / 1000) - clk0;

			if (clk >= clkb + 2)
				{
				clkb = clk / 2 * 2;
				break;
				}
			}

		// 単データリクエスト（次回分）
		WriteFile(fdc, "R", 1, &n, 0);

		// 単データを得る
		n = 0;
		ReadFile(fdc, str, 27, &n, 0);
		
		if (n < 27)
			{
//			printf ("=== error ! n = %d ===\n", n);
			goto skip;
			}

		sscanf(str, "%1d%4hx%4hx%4hx%4hx%4hx%4hx",
			 &tick, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);

		for(i=0;i<6;i++){
			if(i<3)
				zdata[j][i]=data[i]*200.0/16384;
			else
				zdata[j][i]=data[i]*4.0/16384;
			printf("%8.3f",zdata[j][i]);
		
			if(j==0)
				zdata[0][i]=0;
			else{
				zdata[0][i]=zdata[0][i]+zdata[j][i];
				z[i]=zdata[0][i]/j;
			}
		}
		printf("\n");
	}

	// 単データリクエスト（初回分）
	WriteFile(fdc, "R", 1, &n, 0);

	printf("Enter File name > ");
	scanf("%s", fname);
	strcat(fname,csv);
	fd = fopen(fname, "w");
	if (!fd)
		goto over;

	sprintf(str, "Time[ms],Fx[N],Fy[N],Fz[N],Mx[N・m],My[N・m],Mz[N・m]\n");
	fprintf(fd, str);

	// データを読み出す.
	printf("=== record data ===\n");
	clk0 = clock() / (CLOCKS_PER_SEC / 1000);
	clkb = 0;
	clkb2 = 0;
	num = 0;

	while (true)
		{
		// サンプリング周期だけ待つ
		while (true)
			{
			clk = clock() / (CLOCKS_PER_SEC / 1000) - clk0;

			if (clk >= clkb + tw)
				{
				clkb = clk / tw * tw;
				break;
				}
			}

		// 単データリクエスト（次回分）
		WriteFile(fdc, "R", 1, &n, 0);

		// 単データを得る
		n = 0;
		ReadFile(fdc, str, 27, &n, 0);
		
		if (n < 27)
			{
			printf ("=== error ! n = %d ===\n", n);
			goto skip;
			}

		sscanf(str, "%1d%4hx%4hx%4hx%4hx%4hx%4hx",
			 &tick, &data[0], &data[1], &data[2], &data[3], &data[4], &data[5]);

		for(j=1;j<10;j++){
			for(i=0;i<6;i++)
				hdata[j][i]=hdata[j-1][i];
		}

		for(i=0;i<6;i++){
			if(i<3)
				ndata[i]=data[i]*200.0/16384-z[i];
			else
				ndata[i]=data[i]*4.0/16384-z[i];
			hdata[10][i]=ndata[i];
		}
		
		sprintf(str, "%7.3f,%8.3f,%8.3f,%8.3f,%8.3f,%8.3f,%8.3f\n",
			clk / tw * tw/1000.0,
			ndata[0], ndata[1], ndata[2], ndata[3], ndata[4], ndata[5]);

		fprintf(fd, str);
		num++;

skip :
		if (clk >= ts*1000)
			break;

		// コンソールに間引き表示
		if (clk >= clkb2 + 1000)
			{
			printf(str);
			if (_kbhit())
				break;
			clkb2 = clk / 1000 * 1000;
			}
		}

over :
	if (fd)
		{
		fclose(fd);
		fd = NULL;
		}

	if (fdc)
		{
		EscapeCommFunction(fdc, CLRDTR);	// Clear the DTR line
		CloseHandle(fdc);
		fdc = NULL;
		}

	printf ("=== num = %d ===\n", num);

	/*printf("exit (y / n) ? > ");
	scanf("%s", str);
	if (str[0] == 'y')
		{
//		exit(0);
		}
	else
		{
		goto start;
		}*/
	}


int SetComAttr(HANDLE fdc)
	{
	int				status;
	BOOL			success;
	COMMTIMEOUTS	timeouts;
	DCB				dcb;

	status = 0;

	timeouts.ReadIntervalTimeout = 1; 
	timeouts.ReadTotalTimeoutMultiplier = 1;
	timeouts.ReadTotalTimeoutConstant = 1;
	timeouts.WriteTotalTimeoutMultiplier = 1;
	timeouts.WriteTotalTimeoutConstant = 1;

	SetCommTimeouts(fdc, &timeouts);		//  Read Write のタイムアウトの設定
	EscapeCommFunction(fdc, SETDTR);		// Set the Data Terminal Ready line
	
	// Get the current settings of the COMM port 
	success = GetCommState(fdc, &dcb);
	if(!success)
		{
		status = -1;
		}

	// Modify the baud rate, etc.
	dcb.BaudRate = 921600;
	dcb.ByteSize = (BYTE)8;
	dcb.Parity = (BYTE)NOPARITY;
	dcb.StopBits = (BYTE)ONESTOPBIT;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fRtsControl = RTS_CONTROL_TOGGLE;

	// Apply the new comm port settings
	success = SetCommState(fdc, &dcb);
	if(!success)
		{
		status = -2;
		}

	return (status);
	}
