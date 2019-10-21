// Henry Bergin 2019
#include "image_loader.h"
#include <stdio.h>
#include <stdlib.h>

unsigned char *loadBMP(const char *imagepath)
{
	// Data read from the header of the BMP file
	unsigned char header[54]; // Each BMP file begins by a 54-bytes header
	unsigned int dataPos;     // Position in the file where the actual data begins
	unsigned int width, height;
	unsigned int imageSize;   // = width*height*3
	// Actual RGB data
	unsigned char *data;

	// Open the file
	FILE * file = fopen(imagepath, "rb");
	if (!file){printf("Image could not be opened\r\n"); return NULL;}

	if ( fread(header, 1, 54, file)!=54 )
	{ // If not 54 bytes read : problem
    printf("Not a correct BMP file\r\n");
    return NULL;
	}

	if ( header[0]!='B' || header[1]!='M' )
	{
    printf("Not a correct BMP file\n");
    return NULL;
	}

	// Read ints from the byte array
	dataPos    = *(int*)&(header[0x0A]);
	imageSize  = *(int*)&(header[0x22]);
	width      = *(int*)&(header[0x12]);
	height     = *(int*)&(header[0x16]);

	// Some BMP files are misformatted, guess missing information
	if (imageSize==0)    imageSize=width*height*3; // 3 : one byte for each Red, Green and Blue component
	if (dataPos==0)      dataPos=54; // The BMP header is done that way

	// Create a buffer
	data = (unsigned char *)malloc(imageSize);

	// Read the actual data from the file into the buffer
	fread(data, 1, imageSize, file);

	//Everything is in memory now, the file can be closed
	fclose(file);

	printf("image loaded: %d x %d:  %d bytes\r\n", width, height, imageSize);

	// swap red and blue since it's in little endian
	unsigned char temp = 0;
	for(int i = 0; i < imageSize;)
	{
		temp = data[i];
		data[i] = data[i+2];
		data[i+2] = temp;
		i+= 3;
	}

	return data;
}
