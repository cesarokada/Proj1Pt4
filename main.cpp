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

#define wWIDTH  640
#define wHEIGHT 480

#define CP_MAXVERT 32
#define CP_MAXPOLY 32
#define CP_LEFT 0
#define CP_RIGHT 1
#define CP_TOP 2
#define CP_BOTTOM 3

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

/* Program entry point */
int main(int argc, char *argv[])
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

    // inicializar os vertices da janela de recorte
    clipRect.t = 10;
    clipRect.b = 6;
    clipRect.l = 3;
    clipRect.r = 11;

    glutInit(&argc, argv);
    glutInitWindowSize(wWIDTH,wHEIGHT);
    glutInitWindowPosition(10,10);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);

    glutCreateWindow("Algoritmo de Sutherland-Hodgman");

    // Define a Cor de Fundo (aqui branco)
    glClearColor(1.0, 1.0, 1.0, 0.0);

    // Define o Modo de Trabalho
    // (aqui desejamos projetar a imagem no 2D)
    glMatrixMode(GL_PROJECTION);

    // Projetamos a imagem
    // (definindo as coordenadas de mundo desejadas)
    gluOrtho2D(0.0, 16.0, 0.0, 13.0);

    glutDisplayFunc(display);
    glutKeyboardFunc(keyboard);

    glutMainLoop();

    return EXIT_SUCCESS;
}
