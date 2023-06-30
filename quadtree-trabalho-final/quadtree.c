#include "quadtree.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>     /* OpenGL functions */
#endif

unsigned int first = 1;
char desenhaBorda = 1;

QuadNode *newNode(int x, int y, int width, int height)
{
    QuadNode *n = malloc(sizeof(QuadNode));
    n->x = x;
    n->y = y;
    n->width = width;
    n->height = height;
    n->NW = n->NE = n->SW = n->SE = NULL;
    n->color[0] = n->color[1] = n->color[2] = 0;
    n->id = first++;
    return n;
}

QuadNode *geraQuadtree(Img* pic, float minError, int x, int y, float width, float height)
{
    // Converte o vetor RGBPixel para uma MATRIZ que pode acessada por pixels[linha][coluna]
    RGBPixel (*pixels)[pic->width] = (RGBPixel(*)[pic->height]) pic->img;

    int i, j;
    int histo[256];
    int mediaR = 0, mediaG = 0, mediaB = 0;

    for(i = y; i < height + y; i++){
        for(j = x; j < width + x; j++){
            mediaR += pixels[i][j].r;
            mediaG += pixels[i][j].g;
            mediaB += pixels[i][j].b;
        }
    }
    mediaR /= height*width;
    mediaG /= height*width;
    mediaB /= height*width;

    int **imgCinza = (int**)malloc(pic->height * sizeof(int*));

    for(i = 0; i < pic->height; i++){
        imgCinza[i] = (int*)malloc(pic->width * sizeof(int));
    }

    for(i = y; i < height + y; i++){
        for(j = x; j < width + x; j++){
            imgCinza[i][j] = round(0.3*pixels[i][j].r +  0.59*pixels[i][j].g + 0.11*pixels[i][j].b);
        }    
    }
    
    for(i = 0; i < 256; i++){
        histo[i] = 0;
    }

    for(i = y; i < height + y; i++){    
        for(j = x; j < width + x; j++){
            histo[imgCinza[i][j]] += 1;
        }
    }
    

    float intensidade = 0;

    for(i = 0; i < 256; i++){    
        intensidade += histo[i]*i;
    }

    intensidade = intensidade / (width*height);
    
    double somatorio = 0;

    for(i = y; i < height + y -1; i++){    
        for(j = x; j < width + x -1; j++){    
            somatorio = somatorio + ((imgCinza[i][j] - intensidade) * (imgCinza[i][j] - intensidade));
        }
    }

    float erroMedio = sqrt(somatorio/(width*height));

    if(erroMedio > minError){

        QuadNode *raiz = newNode(x,y,width,height);
        raiz->status = PARCIAL;
        raiz->color[0] = mediaR;
        raiz->color[1] = mediaG;
        raiz->color[2] = mediaB;
        
        float meiaLargura = width/2; 
        float meiaAltura = height/2; 

        raiz->NW = geraQuadtree(pic, minError, x, y, meiaLargura, meiaAltura);

        raiz->NE = geraQuadtree(pic, minError, x + meiaLargura, y, meiaLargura, meiaAltura);

        raiz->SE = geraQuadtree(pic, minError, x + meiaLargura, y + meiaAltura, meiaLargura, meiaAltura);

        raiz->SW = geraQuadtree(pic, minError, x, y + meiaAltura, meiaLargura, meiaAltura);


        for (i = 0; i < pic->height; i++) {
            free (imgCinza[i]);
        }
        free (imgCinza);        

        return raiz;
    }
    else{

        QuadNode *raiz = newNode(x,y,width,height);
        raiz->color[0] = mediaR;
        raiz->color[1] = mediaG;
        raiz->color[2] = mediaB;
        raiz->status = CHEIO;

        for (i = 0; i < pic->height; i++) {
            free (imgCinza[i]);
        }
        free (imgCinza);    

        return raiz;
    }
}

// Limpa a memória ocupada pela árvore
void clearTree(QuadNode *n)
{
    if(n == NULL) return;
    if(n->status == PARCIAL)
    {
        clearTree(n->NE);
        clearTree(n->NW);
        clearTree(n->SE);
        clearTree(n->SW);
    }
    //printf("Liberando... %d - %.2f %.2f %.2f %.2f\n", n->status, n->x, n->y, n->width, n->height);
    free(n);
}

// Ativa/desativa o desenho das bordas de cada região
void toggleBorder() {
    desenhaBorda = !desenhaBorda;
    printf("Desenhando borda: %s\n", desenhaBorda ? "SIM" : "NÃO");
}

// Desenha toda a quadtree
void drawTree(QuadNode *raiz) {
    if(raiz != NULL)
        drawNode(raiz);
}

// Grava a árvore no formato do Graphviz
void writeTree(QuadNode *raiz) {
    FILE *fp = fopen("quad.dot", "w");
    fprintf(fp, "digraph quadtree {\n");
    if (raiz != NULL)
        writeNode(fp, raiz);
    fprintf(fp, "}\n");
    fclose(fp);
    printf("\nFim!\n");
}

void writeNode(FILE *fp, QuadNode *n)
{
    if(n == NULL) return;

    if(n->NE != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->NE->id);
    if(n->NW != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->NW->id);
    if(n->SE != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->SE->id);
    if(n->SW != NULL) fprintf(fp, "%d -> %d;\n", n->id, n->SW->id);
    writeNode(fp, n->NE);
    writeNode(fp, n->NW);
    writeNode(fp, n->SE);
    writeNode(fp, n->SW);
}

// Desenha todos os nodos da quadtree, recursivamente
void drawNode(QuadNode *n)
{
    if(n == NULL) return;

    glLineWidth(0.1);

    if(n->status == CHEIO) {
        glBegin(GL_QUADS);
        glColor3ubv(n->color);
        glVertex2f(n->x, n->y);
        glVertex2f(n->x+n->width-1, n->y);
        glVertex2f(n->x+n->width-1, n->y+n->height-1);
        glVertex2f(n->x, n->y+n->height-1);
        glEnd();
    }

    else if(n->status == PARCIAL)
    {
        if(desenhaBorda) {
            glBegin(GL_LINE_LOOP);
            glColor3ubv(n->color);
            glVertex2f(n->x, n->y);
            glVertex2f(n->x+n->width-1, n->y);
            glVertex2f(n->x+n->width-1, n->y+n->height-1);
            glVertex2f(n->x, n->y+n->height-1);
            glEnd();
        }
        drawNode(n->NE);
        drawNode(n->NW);
        drawNode(n->SE);
        drawNode(n->SW);
    }
    // Nodos vazios não precisam ser desenhados... nem armazenados!
}

