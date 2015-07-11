/*
Programa : Trabalho 2 - Computaзгo Grбfica
   Autor : Lucas Schiolin Silveira
  Teclas :
          S : exibe a janela de recorte
          V : exibe os vertices do poligono
          R : recorta o desenho usando Sutherland-Hodgman
          ESC : sai do programa
*/
#include <GL/glut.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <stdio.h>

#define wWIDTH  640
#define wHEIGHT 480

#define CP_MAXVERT 32
#define CP_MAXPOLY 32
#define CP_LEFT 0
#define CP_RIGHT 1
#define CP_TOP 2
#define CP_BOTTOM 3

#define NUMLINHAS 5

struct rect{
    double t; // Top
    double b; // Bottom
    double l; // Left
    double r; // Right
};

struct point{
    double x;
    double y;
};

// vetor de pontos que representam o poligono
struct point vPoligono[CP_MAXPOLY][CP_MAXVERT];
int vPoligonoSize = 5;
int vPolySize[CP_MAXPOLY];

struct point vLinha[NUMLINHAS][2];
int codigos[NUMLINHAS][2];

struct rect clipRect;

// flags de controle
int showClipRect = 0;
int doClip = 0;
int showVerts = 0;
int poligonoAtual = 0;

// funcao que verifica se o ponto P esta dentro da janela R, em relacao ao lado SIDE
int cp_inside( struct point p, struct rect r, int side )
{
    switch( side ){
        case CP_LEFT:
            return p.x >= r.l;
        case CP_RIGHT:
            return p.x <= r.r;
        case CP_TOP:
            return p.y <= r.t;
        case CP_BOTTOM:
            return p.y >= r.b;
    }
}

// funcao que localiza ponto de interseccao do segmento PQ com a janela R, em relacao ao lado SIDE
struct point cp_intersect( struct point p, struct point q, struct rect r, int side )
{
    struct point t;
    double a, b;

    /* find slope and intercept of segment pq */
    a = ( q.y - p.y ) / ( q.x - p.x );
    b = p.y - p.x * a;

    switch( side ){
        case CP_LEFT:
            t.x = r.l;
            t.y = t.x * a + b;
            break;
        case CP_RIGHT:
            t.x = r.r;
            t.y = t.x * a + b;
            break;
        case CP_TOP:
            t.y = r.t;
            t.x = isfinite(a) ? ( t.y - b ) / a : p.x;
            break;
        case CP_BOTTOM:
            t.y = r.b;
            t.x = isfinite(a) ? ( t.y - b ) / a : p.x;
            break;
    }

    return t;
}

// funcao que recorta de fato o poligono, com relacao a um lado da janela de recorte
// (retorna em *v o novo tamanho do vetor de pontos, e altera diretamente o vetor
// de pontos passado por parametro)
void cp_clipplane( int *v, struct point in[CP_MAXVERT], struct rect r, int side )
{
    int i, j=0;
    struct point s, p;
    struct point out[CP_MAXVERT];

    // inicialmente, s vai ser o ultimo ponto do poligono
    s = in[*v-1];

    // para cada ponto do poligono, verificar a cada 2 pontos (P e S),
    // a orientacao da aresta PS do poligono, em relacao a janela de recorte R,
    // considerando um de seus lados
    for( i = 0 ; i < *v ; i++ ){
        p = in[i];  // p = ponto da iteracao atual

        // se P esta dentro da janela de recorte
        if( cp_inside( p, r, side ) ){
            // se S nao esta dentro da janela de recorte
            if( !cp_inside( s, r, side ) ){
                // a aresta PS corta a janela R de dentro para fora,
                // entao deve obter o ponto de interseccao
                out[j] = cp_intersect( p, s, r, side );
                j++;
            }
            // acrescentar o ponto P, pois ele esta dentro da janela
            out[j] = p; j++;
        // se P esta fora da janela, mas S esta dentro
        }else if( cp_inside( s, r, side ) ){
            // a aresta PS corta a janela R de fora para dentro,
            // entao deve obter o ponto de interseccao
            out[j] = cp_intersect( s, p, r, side );
            j++;
        }

        s = p; // alterar s, que sempre vai ser o ponto "i-1"
    }


    *v = j; // indicar o novo tamanho do vetor de pontos (pode ter sido alterado)
    // copiar todos os pontos obtidos para o vetor de pontos que foi informado inicialmente
    for( i = 0 ; i < *v ; i++ ){
        in[i] = out[i];
    }
}

// funcao de recorte Sutherland-Hodgman
void clipSH( int *v, int v2[CP_MAXPOLY], struct point p[CP_MAXPOLY][CP_MAXVERT], struct rect r )
{
    /*
      Esta funcao vai chamar a funcao cp_clipplane, que vai de fato fazer o recorte
      do poligono, alterar o vetor de vertices que foi passado por parametro,
      tudo com base em um lado da janela de recorte.
    */

    // recortar a esquerda
    cp_clipplane( &v2[poligonoAtual], p[poligonoAtual], r, CP_LEFT );
    // recortar a direita
    cp_clipplane( &v2[poligonoAtual], p[poligonoAtual], r, CP_RIGHT );
    // recortar em baixo
    cp_clipplane( &v2[poligonoAtual], p[poligonoAtual], r, CP_BOTTOM );
    // recortar em cima
    cp_clipplane( &v2[poligonoAtual], p[poligonoAtual], r, CP_TOP );
}

// funcao que calcula o produto interno do vetores, formados por 3 pontos
static double prodVet(struct point p1, struct point p2, struct point p3)
{
  double u1, u2, v1, v2;
  // vetor u
  u1 = p1.x - p2.x;
  u2 = p1.y - p2.y;
  // vetor v
  v1 = p3.x - p2.x;
  v2 = p3.y - p2.y;

  // calculo do determinante
  return ((u1 * v2) - (u2 * v1));
}

// funcao que escreve um texto na tela
void drawBitmapText(char *string,float x,float y)
{
  char *c;
  glRasterPos2f(x,y);
  for (c=string; *c != '\0'; c++)   {
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
  }
}

int codificaPonto(double x, double y)
{
    int cod = 0;

    if (y > clipRect.t) //Ponto acima da Janela de Seleção
        cod = 8;
    else if (y < clipRect.b) //Ponto abaixo da Janela de Seleção
        cod = 4;

    if (x > clipRect.r) //Ponto a direita da Janela de Seleção
        cod += 2;
    else if (x < clipRect.l) //Ponto à esquerda da Janela de Seleção
        cod += 1;

    return cod;
}

void comparaCodigos(struct point (*temp)[2])
{
    int i, j, cod;

    for(i = 0; i < NUMLINHAS; i++)
    {
        if ((codigos[i][0] | codigos[i][1]) == 0)
            continue;
        else if((codigos[i][0] & codigos[i][1] != 0))
        {
            temp[i][0].x = -1;
            temp[i][0].y = -1;
            temp[i][1].x = -1;
            temp[i][1].y = -1;
        }
        else
        {
            cod = codigos[i][0] >> 1;
        }

    }
}


void drawLiang(){
    int i,j, u1 = 0, u2 =1, accept = 1;
    double dx = 0 ,dy = 0,r;

    // limpar a tela
    glClear(GL_COLOR_BUFFER_BIT);

    // transferir os pontos da linha original para variaveis temporarias
    // para nao alterar a fonte
    struct point tempLinhas[NUMLINHAS][2];
    for (i = 0; i < NUMLINHAS; i++)
        for (j = 0; j < 2; j++)
            tempLinhas[i][j] = vLinha[i][j];

    if (doClip)
    {
        double p[4],q[4];

        for (i = 0; i < NUMLINHAS; i++){
            dx = tempLinhas[i][1].x - tempLinhas[i][0].x;
            dx = tempLinhas[i][1].y - tempLinhas[i][0].y;

            p[0] = -dx;
            q[0] = tempLinhas[i][i].x - clipRect.l;

            p[1] = dx;
            q[1] = clipRect.r - tempLinhas[i][i].x;

            p[2] = -dy;
            q[2] = tempLinhas[i][i].y - clipRect.b;

            p[3] = dy;
            q[3] = clipRect.t - tempLinhas[i][i].y;

            for(j = 0; j < 4; j++){
                if(p[j] == 0){
                    if(q[j] < 0)
                        accept = 0; //fazer metodo pra descartar linha;
                }
                else{

                    r = q[j]/p[j];

                    if(p[j] < 0){
                        if(r < p[j]){
                            u1 = p[j];
                        }
                        else
                            u1 = r;
                    }
                    else{
                        if(r < u2)
                           u2 = r;
                    }

                    if(u1 < u2)
                        accept = 0;
                }

             }

            if(accept){

                if(tempLinhas[i][0].x < clipRect.l)
                    tempLinhas[i][0].x = clipRect.l;
                if(tempLinhas[i][1].x > clipRect.r)
                    tempLinhas[i][1].x = clipRect.r;

                if(u2 < 1){
                    vLinha[i][1].x = tempLinhas[i][0].x + u2 * dx;
                    vLinha[i][1].y = tempLinhas[i][0].y + u2 * dy;
                }

                if(u1 > 0){
                    vLinha[i][0].x = tempLinhas[i][0].x + u1 * dx;
                    vLinha[i][0].y = tempLinhas[i][0].y + u1 * dy;
                }

           }
        }

        //comparaCodigos(tempLinhas);
       glColor3f(0.0, 0.0, 1.0);
    }
    else
        glColor3f(1.0, 0.0, 0.0);

    for(i = 0; i < NUMLINHAS; i++)
    {
        glBegin(GL_LINE_LOOP);
          glVertex2d((int)tempLinhas[i][0].x, (int)tempLinhas[i][0].y);
          glVertex2d((int)tempLinhas[i][1].x, (int)tempLinhas[i][1].y);
        glEnd();
        glFlush();
    }

    // ***** desenhar a janela de recorte
    if (showClipRect)
    {
      // (cinza)
      glColor3f(0.9, 0.9, 0.9);
      glBegin(GL_LINE_LOOP);
        glVertex2i((int)clipRect.l, (int)clipRect.b);
        glVertex2i((int)clipRect.l, (int)clipRect.t);
        glVertex2i((int)clipRect.r, (int)clipRect.t);
        glVertex2i((int)clipRect.r, (int)clipRect.b);
      glEnd();
      glFlush();
    }

    // ***** escrever a legenda
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.0, 0.0, 0.0);
    drawBitmapText("ESC : sair | E : exibir/ocultar janela recorte | R : recortar",0.1,0.2);
    glFlush();
}


void drawLinesCohen()
{
    int i,j;

    // limpar a tela
    glClear(GL_COLOR_BUFFER_BIT);

    // transferir os pontos da linha original para variaveis temporarias
    // para nao alterar a fonte
    struct point tempLinhas[NUMLINHAS][2];
    for (i = 0; i < NUMLINHAS; i++)
        for (j = 0; j < 2; j++)
            tempLinhas[i][j] = vLinha[i][j];

    if (doClip)
    {
        for (i = 0; i < NUMLINHAS; i++)
            for (j = 0; j < 2; j++)
                codigos[i][j] = codificaPonto(tempLinhas[i][j].x, tempLinhas[i][j].y);
        comparaCodigos(tempLinhas);
       glColor3f(0.0, 0.0, 1.0);
    }
    else
        glColor3f(1.0, 0.0, 0.0);

    for(i = 0; i < NUMLINHAS; i++)
    {
        glBegin(GL_LINE_LOOP);
          glVertex2d((int)tempLinhas[i][0].x, (int)tempLinhas[i][0].y);
          glVertex2d((int)tempLinhas[i][1].x, (int)tempLinhas[i][1].y);
        glEnd();
        glFlush();
    }

    // ***** desenhar a janela de recorte
    if (showClipRect)
    {
      // (cinza)
      glColor3f(0.9, 0.9, 0.9);
      glBegin(GL_LINE_LOOP);
        glVertex2i((int)clipRect.l, (int)clipRect.b);
        glVertex2i((int)clipRect.l, (int)clipRect.t);
        glVertex2i((int)clipRect.r, (int)clipRect.t);
        glVertex2i((int)clipRect.r, (int)clipRect.b);
      glEnd();
      glFlush();
    }

    // ***** escrever a legenda
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.0, 0.0, 0.0);
    drawBitmapText("ESC : sair | E : exibir/ocultar janela recorte | R : recortar",0.1,0.2);
    glFlush();
}

static void display(void)
{
    int i, j;

    // limpar a tela
    glClear(GL_COLOR_BUFFER_BIT);

    // transferir os pontos do poligono original, para variaveis temporarias
    // para nao alterar a fonte
    int size = vPoligonoSize;
    struct point tempPoligono[CP_MAXPOLY][CP_MAXVERT];
    int size2[CP_MAXPOLY];
    for (i = 0; i < vPoligonoSize; i++) {
      for (j = 0; j < vPolySize[i]; j++) {
        tempPoligono[i][j] = vPoligono[i][j];
        size2[j] = vPolySize[j];
      }
    }

    // ***** desenhar o poligono
    if (doClip)
    {
      // recortar com Sutherland-Hodgman
      clipSH(&size, size2, tempPoligono, clipRect);
      // (azul)
      glColor3f(0.0, 0.0, 1.0);
    }
    else
    {
      // (vermelho)
      glColor3f(1.0, 0.0, 0.0);
    }

    // ***** desenhar o poligono
    glBegin(GL_POLYGON);
    for (j = 0; j < size2[poligonoAtual]; j++) {
      glVertex2f(tempPoligono[poligonoAtual][j].x, tempPoligono[poligonoAtual][j].y);
    }
    glEnd();
    glFlush();


    // ***** desenhar a janela de recorte
    if (showClipRect)
    {
      // (cinza)
      glColor3f(0.9, 0.9, 0.9);
      glBegin(GL_LINE_LOOP);
        glVertex2i((int)clipRect.l, (int)clipRect.b);
        glVertex2i((int)clipRect.l, (int)clipRect.t);
        glVertex2i((int)clipRect.r, (int)clipRect.t);
        glVertex2i((int)clipRect.r, (int)clipRect.b);
      glEnd();
      glFlush();
    }

    // ***** exibir os vertices do poligono
    if (showVerts) {
      // (preto)
      glColor3f(0.0, 0.0, 0.0);
      glPointSize(5.0);
      glBegin(GL_POINTS);
      for (j = 0; j < size2[poligonoAtual]; j++) {
        glVertex2f(tempPoligono[poligonoAtual][j].x, tempPoligono[poligonoAtual][j].y);
      }
      glEnd();
      glFlush();
    }

    // ***** escrever a legenda
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.0, 0.0, 0.0);
    drawBitmapText("ESC : sair | E : exibir/ocultar janela recorte | V : exibir/ocultar vertices poligono | R : recortar | 1 ate 5 : poligonos ",0.1,0.2);
    glFlush();

    // ***** verificar se o poligono eh concavo ou convexo
    // passar por todos os pontos do poligono, para verificar se eh convexo
    // (verificar a cada 3 pontos, se os vetores formados por esses pontos tem prod vet. < 0)
    for( i = 0 ; i < vPolySize[poligonoAtual] - 1 ; i++ ){
        // calcular o produto vetorial de dois vetores,
        // formados por tres pontos do poligono
        if (prodVet(vPoligono[poligonoAtual][i],
                    vPoligono[poligonoAtual][i+1],
                    vPoligono[poligonoAtual][(i+2 == vPolySize[poligonoAtual]) ? 0 : i+2]) < 0) {
          // se o produto vetorial eh menor que zero, entao o poligono eh concavo
          glLoadIdentity();
          glColor3f(1.0, 0.0, 0.0);
          drawBitmapText("Poligono nao eh convexo!",0.1,0.7);
          glFlush();
          break;
        }
    } // for
    glMatrixMode(GL_PROJECTION);
}

// funcao que captura eventos do teclado
static void keyboard(unsigned char key, int x, int y)
{
  switch(key)
  {
      case 'e':
          // exibir/ocultar janela de recorte
          showClipRect = (showClipRect == 0) ? 1 : 0;
          display();
          break;
      case 'r':
          // recortar ou nao, o poligono
          doClip = (doClip == 0) ? 1 : 0;
          display();
          break;
      case 'v':
          // exibir vertices
          showVerts = (showVerts == 0) ? 1 : 0;
          display();
          break;
      case '1':
          // poligono 1 - convexo
          poligonoAtual = 0;
          display();
          break;
      case '2':
          // poligono 2 - convexo
          poligonoAtual = 1;
          display();
          break;
      case '3':
          // poligono 3 - concavo
          poligonoAtual = 2;
          display();
          break;
      case '4':
          // poligono 4 - convexo
          poligonoAtual = 3;
          display();
          break;
      case '5':
          // poligono 5 - concavo
          poligonoAtual = 4;
          display();
          break;
      case 27:
          exit(0);
          break;
      case 13:
          // recortar
          break;
  }
}

// funcao que captura eventos do teclado
static void keyboardCohen(unsigned char key, int x, int y)
{
  switch(key)
  {
      case 'e':
          // exibir/ocultar janela de recorte
          showClipRect = (showClipRect == 0) ? 1 : 0;
          drawLinesCohen();
          break;
      case 'r':
          // recortar ou nao, o poligono
          doClip = (doClip == 0) ? 1 : 0;
          drawLinesCohen();
          break;
      case 27:
          exit(0);
          break;
      case 13:
          // recortar
          break;
  }
}

void defineLinhas()
{
    vLinha[0][0].x = 2;   vLinha[0][0].y = 9.5;
    vLinha[0][1].x = 3.5; vLinha[0][1].y = 12;

    vLinha[1][0].x = 4;   vLinha[1][0].y = 7;
    vLinha[1][1].x = 6;   vLinha[1][1].y = 9;

    vLinha[2][0].x = 8;   vLinha[2][0].y = 8;
    vLinha[2][1].x = 12;  vLinha[2][1].y = 12;

    vLinha[3][0].x = 5;   vLinha[3][0].y = 4;
    vLinha[3][1].x = 13;  vLinha[3][1].y = 12;

    vLinha[4][0].x = 1;   vLinha[4][0].y = 3;
    vLinha[4][1].x = 14;  vLinha[4][1].y = 4;
}

void definePoligonos()
{
    // poligono 1 - convexo
    vPoligono[0][0].x = 5;  vPoligono[0][0].y = 4;
    vPoligono[0][1].x = 2;  vPoligono[0][1].y = 9;
    vPoligono[0][2].x = 7; vPoligono[0][2].y = 11;
    vPoligono[0][3].x = 12; vPoligono[0][3].y = 8;
    vPolySize[0] = 4;

    // poligono 2 - convexo
    vPoligono[1][0].x = 4;  vPoligono[1][0].y = 4;
    vPoligono[1][1].x = 4;  vPoligono[1][1].y = 11;
    vPoligono[1][2].x = 11; vPoligono[1][2].y = 11;
    vPoligono[1][3].x = 11; vPoligono[1][3].y = 4;
    vPolySize[1] = 4;

    // poligono 3 - concavo
    vPoligono[2][0].x = 3;  vPoligono[2][0].y = 4;
    vPoligono[2][1].x = 5;  vPoligono[2][1].y = 11;
    vPoligono[2][2].x = 12; vPoligono[2][2].y = 8;
    vPoligono[2][3].x = 9;  vPoligono[2][3].y = 5;
    vPoligono[2][4].x = 5;  vPoligono[2][4].y = 6;
    vPolySize[2] = 5;

    // poligono 4 - convexo
    vPoligono[3][0].x = 5;  vPoligono[3][0].y = 4;
    vPoligono[3][1].x = 2;  vPoligono[3][1].y = 9;
    vPoligono[3][2].x = 7;  vPoligono[3][2].y = 11;
    vPoligono[3][3].x = 12; vPoligono[3][3].y = 6;
    vPolySize[3] = 4;

    // poligono 5 - concavo
    vPoligono[4][0].x = 7;  vPoligono[4][0].y = 4;
    vPoligono[4][1].x = 5;  vPoligono[4][1].y = 7;
    vPoligono[4][2].x = 2;  vPoligono[4][2].y = 8;
    vPoligono[4][3].x = 5;  vPoligono[4][3].y = 9;
    vPoligono[4][4].x = 7;  vPoligono[4][4].y = 12;
    vPoligono[4][5].x = 9;  vPoligono[4][5].y = 9;
    vPoligono[4][6].x = 12; vPoligono[4][6].y = 8;
    vPoligono[4][7].x = 9;  vPoligono[4][7].y = 7;
    vPolySize[4] = 8;
}

void defineJanelaRecorte()
{
    // inicializar os vertices da janela de recorte
    clipRect.t = 10;
    clipRect.b = 6;
    clipRect.l = 3;
    clipRect.r = 11;
}

/* Program entry point */
int main(int argc, char *argv[])
{
    int opcao;

    defineLinhas();
    definePoligonos();
    defineJanelaRecorte();

    printf("Digite a opção desejada:\n");
    printf("1 - Algoritmo Cohen-Sutherland (Recorte de Linhas)\n");
    printf("2 - Algoritmo Liang-Barsky (Recorte de Linhas)\n");
    printf("3 - Algoritmo Sutherland-Hodgman (Recorte de Poligonos)\n");
    scanf("%d", &opcao);

    glutInit(&argc, argv);
    glutInitWindowSize(wWIDTH,wHEIGHT);
    glutInitWindowPosition(10,10);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    glutCreateWindow("Recorte");

    // Define a Cor de Fundo (aqui branco)
    glClearColor(1.0, 1.0, 1.0, 0.0);

    // Define o Modo de Trabalho
    // (aqui desejamos projetar a imagem no 2D)
    glMatrixMode(GL_PROJECTION);

    // Projetamos a imagem
    // (definindo as coordenadas de mundo desejadas)
    gluOrtho2D(0.0, 16.0, 0.0, 13.0);

    if (opcao == 1)
    {
        glutDisplayFunc(drawLinesCohen);
        glutKeyboardFunc(keyboardCohen);
    }
    else if (opcao == 2)
    {
        glutDisplayFunc(drawLiang);
        glutKeyboardFunc(keyboardCohen);
    }
    else
    {
        glutDisplayFunc(display);
        glutKeyboardFunc(keyboard);
    }

    glutMainLoop();

    return EXIT_SUCCESS;
}
