#include <windows.h>
#include <scrnsave.h>
#include <gl/gl.h>
#include <stdio.h>
#include <sys/types.h>
#include <process.h>
#include <time.h>
#include <stdbool.h>
#include <stdlib.h>

#define WIDTH 256  // 画像の横のピクセル数
#define HEIGHT 256 // 画像の縦のピクセル数

#define LENGTH 300 //文字列の長さ

void EnableOpenGL(void);
void DisableOpenGL(HWND);
HDC hDC;
HGLRC hRC;
int wx, wy;
GLubyte *bits;
int finish = 0;
int min_l, min_r, hour_l, hour_r, sec_r, sec_l;
bool reload_background = true;
int num_b = 0;

void time_f(void)
{
  time_t t = time(NULL);
  struct tm *local = localtime(&t);

  hour_r = (int)(local->tm_hour) % 10;
  hour_l = (int)(local->tm_hour) / 10;
  min_r = (int)(local->tm_min) % 10;
  min_l = (int)(local->tm_min) / 10;
  sec_r = (int)(local->tm_sec) % 10;
  sec_l = (int)(local->tm_sec) / 10;
}

void readBits(GLubyte *bits, char *fileName, int width, int height)
{
  FILE *fp;

  if ((fp = fopen(fileName, "rb")) == NULL)
  { /* 画像データ読み込み */
    fprintf(stderr, "file not found.\n");
    exit(0);
  }
  fseek(fp, 54, SEEK_SET); /* ヘッダ54バイトを読み飛ばす */

  /* 画像を反転させずに読み込み */
  for (int i = 0; i < width * height; i++)
  { /* データをBGRをRGBの順に変えて読み込み */
    fread(&bits[2], 1, 1, fp);
    fread(&bits[1], 1, 1, fp);
    fread(&bits[0], 1, 1, fp);
    bits = bits + 3;
  }
  fclose(fp);
}

void readBitsAlpha(GLubyte *bits, char *fileName, int width, int height)
{
  FILE *fp;

  if ((fp = fopen(fileName, "rb")) == NULL)
  { /* 画像データ読み込み */
    fprintf(stderr, "file not found.\n");
    exit(0);
  }
  fseek(fp, 54, SEEK_SET); /* ヘッダ54バイトを読み飛ばす */

  /* 画像を反転させずに読み込み */
  for (int i = 0; i < width * height; i++)
  { /* データをBGRをRGBの順に変えて読み込み、alpha値を追加 */
    fread(&bits[2], 1, 1, fp);
    fread(&bits[1], 1, 1, fp);
    fread(&bits[0], 1, 1, fp);
    if (bits[2] == 255 && bits[1] == 255 && bits[0] == 255)
      bits[3] = 0; // 白なら透明
    else
      bits[3] = 255; // それ以外は不透明
    bits = bits + 4;
  }
  fclose(fp);
}

void readBitsRev(GLubyte *bits, char *fileName, int width, int height)
{
  FILE *fp;

  if ((fp = fopen(fileName, "rb")) == NULL)
  { /* 画像データ読み込み */
    fprintf(stderr, "file not found.\n");
    exit(0);
  }
  fseek(fp, 54, SEEK_SET); /* ヘッダ54バイトを読み飛ばす */

  // 画像を上下反転させて読み込み。左右は反転させない。
  for (int h = height - 1; h >= 0; h--)
  { // 上下反転
    for (int w = 0; w < width; w++)
    { // 左右は反転させないが、BGRをRGBの順に変えて読み込み
      int i;
      i = (width * h + w) * 3;
      fread(&bits[i + 2], 1, 1, fp);
      fread(&bits[i + 1], 1, 1, fp);
      fread(&bits[i], 1, 1, fp);
    }
  }
  fclose(fp);
}

void drawbackground(GLubyte *bits)
{
  glDrawPixels(1920, 1080, GL_RGB, GL_UNSIGNED_BYTE, bits);
}

void disp_clock_fx(int id, double locx, double locy, double num, GLuint *textureID)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, textureID[id]);
  glBegin(GL_POLYGON);
  glTexCoord2f(0, 0);
  glVertex2f(locx + num, locy - 10.0);
  glTexCoord2f(0, 1);
  glVertex2f(locx + num, locy);
  glTexCoord2f(1, 1);
  glVertex2f(locx + num + 10.0, locy);
  glTexCoord2f(1, 0);
  glVertex2f(locx + num + 10.0, locy - 10.0);
  glEnd();
}

void disp_S_fx(int id, double locx, double locy, double num, GLuint *textureID)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, textureID[id]);
  glBegin(GL_POLYGON);
  glTexCoord2f(0, 0);
  glVertex2f(locx + num, locy - 50.0);
  glTexCoord2f(0, 1);
  glVertex2f(locx + num, locy + 30.0);
  glTexCoord2f(1, 1);
  glVertex2f(locx + num + 80.0, locy + 30.0);
  glTexCoord2f(1, 0);
  glVertex2f(locx + num + 80.0, locy - 50.0);
  glEnd();
}

void disp_fx(int id, double locx, double locy, double num, GLuint *textureID)
{
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, textureID[id]);
  glBegin(GL_POLYGON);
  glTexCoord2f(0, 0);
  glVertex2f(locx + num, locy - 20.0);
  glTexCoord2f(0, 1);
  glVertex2f(locx + num, locy);
  glTexCoord2f(1, 1);
  glVertex2f(locx + num + 20.0, locy);
  glTexCoord2f(1, 0);
  glVertex2f(locx + num + 20.0, locy - 20.0);
  glEnd();
}

void display(char *ssd, GLuint *textureID)
{
  static int i = 0;
  char direction;
  static double locx = 40.0;
  static double locy = 0.0;
  static double clockx = 60.0;
  static double clocky = 40.0;
  static bool isX = false;
  static bool isS = false;
  static int counterS = 0;
  static int numS = 0;
  static double secretx = 80.0;
  static double secrety = 0.0;
  //static int tmp_b = 0; //停止時背景を変更させる場合に必要

  // static int index = 0;

  if (isS)
  {
    if (counterS > 0)
    {
      counterS--;
    }
    else
    {
      isS = false;
    }
  }
  else
  {
    i = i % LENGTH;
    direction = ssd[i];
    i++;
    switch (direction)
    {
    case 'u':
      locy += 0.5;
      break;
    case 'd':
      locy -= 0.5;
      break;
    case 'r':
      locx += 0.5;
      break;
    case 'l':
      locx -= 0.25;
      break;
    case 'D':
      locx -= 0.5;
      break;
    case 'T':
      locx -= 0.75;
      break;
    case 'Q':
      locx -= 1.0;
      break;
    case 'S':
      counterS = 10;
      isS = true;
      break;
    case 'R':
      locx = 40.0;
      locy = 0.0;
      break;
    case 'C':
      locx = 40.0;
      locy = 0.0;
      numS++;
      numS = numS % 3;
      break;
    case 'B':
      num_b++;
      reload_background = true;
      break;
    case 'X':
      isX = true;
      break;
    default: // 指定以外の文字が含まれていたら終了
      fprintf(stderr, "Invalid character.\n");
      exit(0);
    }
    //以下，停止時に背景を変更
    //動作の安定性，視認しやすさなどを総合的に判断し削除
    /*if ((isS == false) && (ssd[i + 1] == 'S'))
    {
      tmp_b = num_b;
      num_b = numS + 1;
      reload_background = true;
    }
    else if ((isS == true) && (ssd[i + 1] != 'S'))
    {
      num_b = tmp_b;
      reload_background = true;
    }*/
  }

  if (locx < -200.0)
  {
    locx = 40.0;
  }

  if (isX)
  {
    if (secretx < -200)
    {
      isX = false;
      secretx = 80.0;
      secrety = 0.0;
      int r = rand() % 50;
      secrety += (double)r;
    }
    else
    {
      secretx -= 0.7;
    }
  }

  glClear(GL_COLOR_BUFFER_BIT);
  drawbackground(bits); // 背景描画

  glEnable(GL_TEXTURE_2D);
  /*
  if (i == 1)
  {
    index = (index + 1) % 2;
    glBindTexture(GL_TEXTURE_2D, textureID[index]);
  }*/

  //コメント描画
  disp_S_fx(11 + numS, locx, locy, 30, textureID);

  //secret描画
  disp_fx(14, secretx, secrety, 30, textureID);

  // 時計描画
  time_f();
  disp_clock_fx(sec_r, clockx, clocky, 0, textureID);
  disp_clock_fx(sec_l, clockx, clocky, -5, textureID);
  disp_clock_fx(min_r, clockx, clocky, -15, textureID);
  disp_clock_fx(min_l, clockx, clocky, -20, textureID);
  disp_clock_fx(hour_r, clockx, clocky, -30, textureID);
  disp_clock_fx(hour_l, clockx, clocky, -35, textureID);
  disp_clock_fx(10, clockx, clocky, -10, textureID);
  disp_clock_fx(10, clockx, clocky, -25, textureID);

  /*if (i < 50)
  {
    disp_fx(10, locx, locy, 21, textureID);
    disp_fx(10, locx, locy, 52.5, textureID);
  }*/

  glDisable(GL_TEXTURE_2D);
}

unsigned __stdcall disp(void *arg)
{
  GLuint textureID[2];
  static char ssd[LENGTH];
  static FILE *fp = NULL;

  EnableOpenGL(); // OpenGL設定

  /* OpenGL初期設定 */
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-50.0 * wx / wy, 50.0 * wx / wy, -50.0, 50.0, -50.0, 50.0); // 座標系設定

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0.1f, 0.2f, 0.3f, 0.0f); // glClearするときの色設定
  glColor3f(0.9, 0.8, 0.7);             // glRectf等で描画するときの色設定
  glViewport(0, 0, wx, wy);
  /* ここまで */

  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // 透明の計算方法の設定
  glEnable(GL_BLEND);                                // 透明処理をONに設定

  /* スクリーンセーバ記述プログラム読み込み */
  fp = fopen("sample1", "r");
  if (fp == NULL)
  {
    fprintf(stderr, "ファイルオープン失敗\n");
    exit(0);
  }
  for (int i = 0; i < LENGTH; i++)
    if (fscanf(fp, "%c", &ssd[i]) == EOF)
    { // 100文字なかったら終了
      fprintf(stderr, "The length of the input is less than %d.\n", LENGTH);
      exit(0);
    }
  fclose(fp);
  /* ここまで */

  /* bitmap画像を読み込み、textureに割り当てる */
  if ((bits = malloc(WIDTH * HEIGHT * 4)) == NULL)
  { /*  画像読み込み用領域確保 */
    fprintf(stderr, "allocation failed.\n");
    exit(0);
  }
  glGenTextures(14, textureID); /* texture idを15個作成し、配列textureIDに格納する */

  /* 1つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[0]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "0.bmp", WIDTH, HEIGHT); /* 1つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 2つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[1]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "1.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 3つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[2]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "2.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 4つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[3]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "3.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 5つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[4]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "4.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 6つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[5]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "5.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 7つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[6]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "6.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 8つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[7]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "7.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 9つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[8]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "8.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 10つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[9]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "9.bmp", WIDTH, HEIGHT); /* 2つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 11つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[10]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "colon.bmp", WIDTH, HEIGHT); /* 11つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 12つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[11]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "S0.bmp", WIDTH, HEIGHT); /* 12つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 13つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[12]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "S1.bmp", WIDTH, HEIGHT); /* 12つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 14つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[13]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "S2.bmp", WIDTH, HEIGHT); /* 12つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* 15つ目のtexture */
  glBindTexture(GL_TEXTURE_2D, textureID[14]);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  readBitsAlpha(bits, "X.bmp", WIDTH, HEIGHT); /* 13つ目の画像読み込み */
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, WIDTH, HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, bits);

  /* ここまで */
  srand((unsigned int)time(NULL)); //rand initialization

  /* 表示関数呼び出しの無限ループ */
  while (1)
  {
    if (reload_background)
    {
      if ((bits = realloc(bits, 1920 * 1080 * 3)) == NULL)
      { /*  背景画像読み込み用領域確保 */
        fprintf(stderr, "allocation failed.\n");
        exit(0);
      }
      switch (num_b % 5)
      {
      case 0:
        readBits(bits, "B0.bmp", 1920, 1080);
        break;
      case 1:
        readBits(bits, "B1.bmp", 1920, 1080);
        break;
      case 2:
        readBits(bits, "B2.bmp", 1920, 1080);
        break;
      case 3:
        readBits(bits, "B3.bmp", 1920, 1080);
        break;
      case 4:
        readBits(bits, "B4.bmp", 1920, 1080);
        break;
      default:
        readBits(bits, "B0.bmp", 1920, 1080);
        break;
      }
      reload_background = false;
    }

    Sleep(1);                // 0.001秒待つ
    display(ssd, textureID); // 描画関数呼び出し
    glFlush();               // 画面描画強制
    SwapBuffers(hDC);        // front bufferとback bufferの入れ替え
    if (finish == 1)         // finishが1なら描画スレッドを終了させる
      return 0;
  }
}

LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static HANDLE thread_id1;
  RECT rc;

  switch (msg)
  {
  case WM_CREATE:
    hDC = GetDC(hWnd);
    GetClientRect(hWnd, &rc);
    wx = rc.right - rc.left;
    wy = rc.bottom - rc.top;

    thread_id1 = (HANDLE)_beginthreadex(NULL, 0, disp, NULL, 0, NULL); // 描画用スレッド生成
    if (thread_id1 == 0)
    {
      fprintf(stderr, "pthread_create : %s", strerror(errno));
      exit(0);
    }
    break;
  case WM_ERASEBKGND:
    return TRUE;
  case WM_DESTROY:
    finish = 1;                                // 描画スレッドを終了させるために1を代入
    WaitForSingleObject(thread_id1, INFINITE); // 描画スレッドの終了を待つ
    CloseHandle(thread_id1);
    DisableOpenGL(hWnd);
    PostQuitMessage(0);
    free(bits);
    return 0;
  default:
    break;
  }
  return DefScreenSaverProc(hWnd, msg, wParam, lParam);
}

BOOL WINAPI ScreenSaverConfigureDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  return TRUE;
}

BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
  return TRUE;
}

void EnableOpenGL(void)
{
  int format;
  PIXELFORMATDESCRIPTOR pfd = {
      sizeof(PIXELFORMATDESCRIPTOR), // size of this pfd
      1,                             // version number
      0 | PFD_DRAW_TO_WINDOW         // support window
          | PFD_SUPPORT_OPENGL       // support OpenGL
          | PFD_DOUBLEBUFFER         // double buffered
      ,
      PFD_TYPE_RGBA,    // RGBA type
      24,               // 24-bit color depth
      0, 0, 0, 0, 0, 0, // color bits ignored
      0,                // no alpha buffer
      0,                // shift bit ignored
      0,                // no accumulation buffer
      0, 0, 0, 0,       // accum bits ignored
      32,               // 32-bit z-buffer
      0,                // no stencil buffer
      0,                // no auxiliary buffer
      PFD_MAIN_PLANE,   // main layer
      0,                // reserved
      0, 0, 0           // layer masks ignored
  };

  /* OpenGL設定 */
  format = ChoosePixelFormat(hDC, &pfd);
  SetPixelFormat(hDC, format, &pfd);
  hRC = wglCreateContext(hDC);
  wglMakeCurrent(hDC, hRC);
  /* ここまで */
}

void DisableOpenGL(HWND hWnd)
{
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(hRC);
  ReleaseDC(hWnd, hDC);
}
