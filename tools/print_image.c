#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *image;
    int num = atoi(argv[1]);
    int off = atoi(argv[2]);
    int i;
    unsigned int ins;
    image = fopen("image", "r");
    for(i = 0; i < num; ++i)
    {
        fseek(image, i*4 + off, SEEK_SET);
        fread(&ins, 4, 1, image);
        printf("%08x\n", ins);
    }

    return 0;
}
