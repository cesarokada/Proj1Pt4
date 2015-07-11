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

#define CL_LEFT 0
#define CL_RIGHT 1
#define CL_BOTTOM 2
#define CL_TOP 3

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

struct point vPoligono[CP_MAXPOLY][CP_MAXVERT];
int vPoligonoSize = 5;
int vPolySize[CP_MAXPOLY];

struct point vLinha[NUMLINHAS][2];
int codigos[NUMLINHAS][2];

struct rect clipRect;

int mostraJanelaRecorte = 0;
int recorta = 0;
int mostraVertices = 0;
int poligonoAtual = 0;

int pontoDentro(struct point p, struct rect r, int side)
{
    switch(side)
    {
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

struct point pontoIntersecta(struct point p, struct point q, struct rect r, int side)
{
    struct point t;
    double a, b;

    a = (q.y - p.y) / (q.x - p.x);
    b = p.y - p.x * a;

    switch(side)
    {
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

void recortaPoligono(int *v, struct point in[CP_MAXVERT], struct rect r, int side)
{
    int i, j=0;
    struct point s, p;
    struct point out[CP_MAXVERT];

    s = in[*v-1];

    for(i = 0 ; i < *v ; i++)
    {
        p = in[i];
        if(pontoDentro(p, r, side))
        {
            if(!pontoDentro(s, r, side))
            {
                out[j] = pontoIntersecta(p, s, r, side);
                j++;
            }
            out[j] = p; j++;
        }
        else if(pontoDentro(s, r, side))
        {
            out[j] = pontoIntersecta(s, p, r, side);
            j++;
        }

        s = p;
    }

    *v = j;

    for(i = 0 ; i < *v ; i++)
        in[i] = out[i];
}

void sutherland(int *v, int v2[CP_MAXPOLY], struct point p[CP_MAXPOLY][CP_MAXVERT], struct rect r)
{
    recortaPoligono(&v2[poligonoAtual], p[poligonoAtual], r, CP_LEFT);
    recortaPoligono(&v2[poligonoAtual], p[poligonoAtual], r, CP_RIGHT);
    recortaPoligono(&v2[poligonoAtual], p[poligonoAtual], r, CP_BOTTOM);
    recortaPoligono(&v2[poligonoAtual], p[poligonoAtual], r, CP_TOP);
}

static double prodVet(struct point p1, struct point p2, struct point p3)
{
    double u1, u2, v1, v2;

    u1 = p1.x - p2.x;
    u2 = p1.y - p2.y;

    v1 = p3.x - p2.x;
    v2 = p3.y - p2.y;

    return ((u1 * v2) - (u2 * v1));
}

void drawBitmapText(char *string,float x,float y)
{
    char *c;
    glRasterPos2f(x,y);
    for (c=string; *c != '\0'; c++)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *c);
}

void desenhaJanelaRecorte()
{
    glColor3f(0.9, 0.9, 0.9);
    glBegin(GL_LINE_LOOP);
    glVertex2i((int)clipRect.l, (int)clipRect.b);
    glVertex2i((int)clipRect.l, (int)clipRect.t);
    glVertex2i((int)clipRect.r, (int)clipRect.t);
    glVertex2i((int)clipRect.r, (int)clipRect.b);
    glEnd();
    glFlush();
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

void trocaPontos(struct point (*pontos)[2], int linha)
{
    struct point temp;

    temp = pontos[linha][0];
    pontos[linha][0] = pontos[linha][1];
    pontos[linha][1] = temp;
}

void trocaCodigos(int (*codigos)[2], int linha)
{
    int temp;

    temp = codigos[linha][0];
    codigos[linha][0] = codigos[linha][1];
    codigos[linha][1] = temp;
}

void comparaCodigos(struct point (*temp)[2], int side)
{
    int i;
    float m;

    for(i = 0; i < NUMLINHAS; i++)
    {
        if ((codigos[i][0] | codigos[i][1]) == 0)
            continue;
        else if((codigos[i][0] & codigos[i][1]) != 0)
        {
            temp[i][0].x = -1;
            temp[i][0].y = -1;
            temp[i][1].x = -1;
            temp[i][1].y = -1;
        }
        else
        {
            if((codigos[i][0]) == 0)
            {
                trocaPontos(temp, i);
                trocaCodigos(codigos, i);
            }

            if (temp[i][1].x != temp[i][0].x)
                m = (temp[i][1].y - temp[i][0].y) / (temp[i][1].x - temp[i][0].x);

            switch(side)
            {
            case CL_LEFT:
                if(codigos[i][0] & 1)
                {
                    temp[i][0].y += m * (clipRect.l - temp[i][0].x);
                    temp[i][0].x = clipRect.l;
                    codigos[i][0] = codificaPonto(temp[i][0].x, temp[i][0].y);
                }
            case CL_RIGHT:
                if(codigos[i][0] & 2)
                {
                    temp[i][0].y += m * (clipRect.r - temp[i][0].x);
                    temp[i][0].x = clipRect.r;
                    codigos[i][0] = codificaPonto(temp[i][0].x, temp[i][0].y);
                }
            case CL_BOTTOM:
                if(codigos[i][0] & 4)
                {
                    if(temp[i][1].x != temp[i][0].x)
                        temp[i][0].x += (clipRect.b - temp[i][0].y) / m;
                    temp[i][0].y = clipRect.b;
                    codigos[i][0] = codificaPonto(temp[i][0].x, temp[i][0].y);
                }
            case CL_TOP:
                if(codigos[i][0] & 8)
                {
                    if(temp[i][1].x != temp[i][0].x)
                        temp[i][0].x += (clipRect.t - temp[i][0].y) / m;
                    temp[i][0].y = clipRect.t;
                    codigos[i][0] = codificaPonto(temp[i][0].x, temp[i][0].y);
                }
            }
        }

    }
}


void drawLiang()
{
    int i,j, u1 = 0, u2 =1, accept = 1;
    double dx = 0 ,dy = 0,r;

    glClear(GL_COLOR_BUFFER_BIT);

    struct point tempLinhas[NUMLINHAS][2];
    for (i = 0; i < NUMLINHAS; i++)
        for (j = 0; j < 2; j++)
            tempLinhas[i][j] = vLinha[i][j];

    if (recorta)
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

    if (mostraJanelaRecorte)
        desenhaJanelaRecorte();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.0, 0.0, 0.0);
    drawBitmapText("ESC : sair | E : exibir/ocultar janela recorte | R : recortar",0.1,0.2);
    glFlush();
}


void drawLinesCohen()
{
    int i,j;

    glClear(GL_COLOR_BUFFER_BIT);

    struct point tempLinhas[NUMLINHAS][2];
    for (i = 0; i < NUMLINHAS; i++)
        for (j = 0; j < 2; j++)
            tempLinhas[i][j] = vLinha[i][j];

    if (recorta)
    {
        for (i = 0; i < NUMLINHAS; i++)
            for (j = 0; j < 2; j++)
                codigos[i][j] = codificaPonto(tempLinhas[i][j].x, tempLinhas[i][j].y);
        for (i = CL_LEFT; i < CL_TOP; i++)
            comparaCodigos(tempLinhas, i);
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

    if (mostraJanelaRecorte)
        desenhaJanelaRecorte();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.0, 0.0, 0.0);
    drawBitmapText("ESC : sair | E : exibir/ocultar janela recorte | R : recortar",0.1,0.2);
    glFlush();
}

static void display(void)
{
    int i, j;

    glClear(GL_COLOR_BUFFER_BIT);

    int size = vPoligonoSize;
    struct point tempPoligono[CP_MAXPOLY][CP_MAXVERT];
    int size2[CP_MAXPOLY];
    for (i = 0; i < vPoligonoSize; i++) {
        for (j = 0; j < vPolySize[i]; j++) {
            tempPoligono[i][j] = vPoligono[i][j];
            size2[j] = vPolySize[j];
        }
    }

    if (recorta)
    {
        sutherland(&size, size2, tempPoligono, clipRect);
        glColor3f(0.0, 0.0, 1.0);
    }
    else
        glColor3f(1.0, 0.0, 0.0);

    glBegin(GL_POLYGON);
    for (j = 0; j < size2[poligonoAtual]; j++)
        glVertex2f(tempPoligono[poligonoAtual][j].x, tempPoligono[poligonoAtual][j].y);
    glEnd();
    glFlush();

    if (mostraJanelaRecorte)
        desenhaJanelaRecorte();

    if (mostraVertices)
    {
        glColor3f(0.0, 0.0, 0.0);
        glPointSize(5.0);
        glBegin(GL_POINTS);
        for (j = 0; j < size2[poligonoAtual]; j++)
            glVertex2f(tempPoligono[poligonoAtual][j].x, tempPoligono[poligonoAtual][j].y);
        glEnd();
        glFlush();
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor3f(0.0, 0.0, 0.0);
    drawBitmapText("ESC : sair | E : exibir/ocultar janela recorte | V : exibir/ocultar vertices poligono | R : recortar ",0.1,0.2);
    glFlush();

    for( i = 0 ; i < vPolySize[poligonoAtual] - 1 ; i++ )
    {
        if (prodVet(vPoligono[poligonoAtual][i],
                    vPoligono[poligonoAtual][i+1],
                    vPoligono[poligonoAtual][(i+2 == vPolySize[poligonoAtual]) ? 0 : i+2]) < 0)
        {
            glLoadIdentity();
            glColor3f(1.0, 0.0, 0.0);
            drawBitmapText("Poligono nao eh convexo!",0.1,0.7);
            glFlush();
            break;
        }
    }
    glMatrixMode(GL_PROJECTION);
}

static void keyboard(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'e':
        mostraJanelaRecorte = (mostraJanelaRecorte == 0) ? 1 : 0;
        display();
        break;
    case 'r':
        recorta = (recorta == 0) ? 1 : 0;
        display();
        break;
    case 'v':
        mostraVertices = (mostraVertices == 0) ? 1 : 0;
        display();
        break;
    case 27:
        exit(0);
        break;
    case 13:
        break;
    }
}

static void keyboardCohen(unsigned char key, int x, int y)
{
    switch(key)
    {
    case 'e':
        mostraJanelaRecorte = (mostraJanelaRecorte == 0) ? 1 : 0;
        drawLinesCohen();
        break;
    case 'r':
        recorta = (recorta == 0) ? 1 : 0;
        drawLinesCohen();
        break;
    case 27:
        exit(0);
        break;
    case 13:
        break;
    }
}

void defineLinhas()
{
    vLinha[0][0].x = 1;   vLinha[0][0].y = 8;
    vLinha[0][1].x = 5;   vLinha[0][1].y = 12;

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

    vPoligono[0][0].x = 7;  vPoligono[0][0].y = 4;
    vPoligono[0][1].x = 5;  vPoligono[0][1].y = 7;
    vPoligono[0][2].x = 2;  vPoligono[0][2].y = 8;
    vPoligono[0][3].x = 5;  vPoligono[0][3].y = 9;
    vPoligono[0][4].x = 7;  vPoligono[0][4].y = 12;
    vPoligono[0][5].x = 9;  vPoligono[0][5].y = 9;
    vPoligono[0][6].x = 12; vPoligono[0][6].y = 8;
    vPoligono[0][7].x = 9;  vPoligono[0][7].y = 7;
    vPolySize[0] = 8;
}

void defineJanelaRecorte()
{
    clipRect.t = 10;
    clipRect.b = 6;
    clipRect.l = 3;
    clipRect.r = 11;
}

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
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glMatrixMode(GL_PROJECTION);
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
