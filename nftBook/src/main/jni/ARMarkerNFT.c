/* 
 *  ARMarkerNFT.c
 *  ARToolKit5
 *
 *  Disclaimer: IMPORTANT:  This Daqri software is supplied to you by Daqri
 *  LLC ("Daqri") in consideration of your agreement to the following
 *  terms, and your use, installation, modification or redistribution of
 *  this Daqri software constitutes acceptance of these terms.  If you do
 *  not agree with these terms, please do not use, install, modify or
 *  redistribute this Daqri software.
 *
 *  In consideration of your agreement to abide by the following terms, and
 *  subject to these terms, Daqri grants you a personal, non-exclusive
 *  license, under Daqri's copyrights in this original Daqri software (the
 *  "Daqri Software"), to use, reproduce, modify and redistribute the Daqri
 *  Software, with or without modifications, in source and/or binary forms;
 *  provided that if you redistribute the Daqri Software in its entirety and
 *  without modifications, you must retain this notice and the following
 *  text and disclaimers in all such redistributions of the Daqri Software.
 *  Neither the name, trademarks, service marks or logos of Daqri LLC may
 *  be used to endorse or promote products derived from the Daqri Software
 *  without specific prior written permission from Daqri.  Except as
 *  expressly stated in this notice, no other rights or licenses, express or
 *  implied, are granted by Daqri herein, including but not limited to any
 *  patent rights that may be infringed by your derivative works or by other
 *  works in which the Daqri Software may be incorporated.
 *
 *  The Daqri Software is provided by Daqri on an "AS IS" basis.  DAQRI
 *  MAKES NO WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION
 *  THE IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE, REGARDING THE DAQRI SOFTWARE OR ITS USE AND
 *  OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS.
 *
 *  IN NO EVENT SHALL DAQRI BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL
 *  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION,
 *  MODIFICATION AND/OR DISTRIBUTION OF THE DAQRI SOFTWARE, HOWEVER CAUSED
 *  AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING NEGLIGENCE),
 *  STRICT LIABILITY OR OTHERWISE, EVEN IF DAQRI HAS BEEN ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Copyright 2015 Daqri LLC. All Rights Reserved.
 *  Copyright 2011-2015 ARToolworks, Inc. All Rights Reserved.
 *
 *  Author(s): Philip Lamb
 *
 */

#include "ARMarkerNFT.h"

#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#  include <windows.h>
#  define MAXPATHLEN MAX_PATH
#else
#  include <sys/param.h> // MAXPATHLEN
#endif


const ARPose ARPoseUnity = {{1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f}};

static char *get_buff(char *buf, int n, FILE *fp, int skipblanks)
{
    char *ret;
	size_t l;
    
    do {
        ret = fgets(buf, n, fp);
        if (ret == NULL) return (NULL); // EOF or error.
        
        // Remove NLs and CRs from end of string.
        l = strlen(buf);
        while (l > 0) {
            if (buf[l - 1] != '\n' && buf[l - 1] != '\r') break;
            l--;
            buf[l] = '\0';
        }
    } while (buf[0] == '#' || (skipblanks && buf[0] == '\0')); // Reject comments and blank lines.
    
    return (ret);
}

void newMarkers(const char *markersConfigDataFilePathC, ARMarkerNFT **markersNFT_out, int *markersNFTCount_out)
{
    FILE          *fp;
    char           buf[MAXPATHLEN], buf1[MAXPATHLEN];
    int            tempI;
    ARMarkerNFT   *markersNFT;
    int            markersNFTCount;
    ARdouble       tempF;
    int            i;
    char           markersConfigDataDirC[MAXPATHLEN];
    size_t         markersConfigDataDirCLen;

    if (!markersConfigDataFilePathC || markersConfigDataFilePathC[0] == '\0' || !markersNFT_out || !markersNFTCount_out) return;
        
    // Load the marker data file.
#ifdef DEBUG
    ARLOGe("Opening marker config. data file from path '%s'.\n", markersConfigDataFilePathC);
#endif
    arUtilGetDirectoryNameFromPath(markersConfigDataDirC, markersConfigDataFilePathC, MAXPATHLEN, 1); // 1 = add '/' at end.
    markersConfigDataDirCLen = strlen(markersConfigDataDirC);
    if ((fp = fopen(markersConfigDataFilePathC, "r")) == NULL) {
        ARLOGe("Error: unable to locate marker config data file '%s'.\n", markersConfigDataFilePathC);
        return;
    }
    
    // First line is number of markers to read.
    get_buff(buf, MAXPATHLEN, fp, 1);
    if (sscanf(buf, "%d", &tempI) != 1 ) {
        ARLOGe("Error in marker configuration data file; expected marker count.\n");
        fclose(fp);
        return;
    }
    
    arMallocClear(markersNFT, ARMarkerNFT, tempI);
    markersNFTCount = tempI;
    
#ifdef DEBUG
    ARLOGe("Reading %d marker configuration(s).\n", markersNFTCount);
#endif

    for (i = 0; i < markersNFTCount; i++) {
        
        // Read marker name.
        if (!get_buff(buf, MAXPATHLEN, fp, 1)) {
            ARLOGe("Error in marker configuration data file; expected marker name.\n");
            break;
        }
        
        // Read marker type.
        if (!get_buff(buf1, MAXPATHLEN, fp, 1)) {
            ARLOGe("Error in marker configuration data file; expected marker type.\n");
            break;
        }
        
        // Interpret marker type, and read more data.
        if (strcmp(buf1, "SINGLE") == 0) {
            ARLOGe("Error in marker configuration data file; SINGLE markers not supported in this build.\n");
        } else if (strcmp(buf1, "MULTI") == 0) {
            ARLOGe("Error in marker configuration data file; MULTI markers not supported in this build.\n");
        } else if (strcmp(buf1, "NFT") == 0) {
            markersNFT[i].valid = markersNFT[i].validPrev = FALSE;
            arMalloc(markersNFT[i].datasetPathname, char, markersConfigDataDirCLen + strlen(buf) + 1);
            strcpy(markersNFT[i].datasetPathname, markersConfigDataDirC);
            strcpy(markersNFT[i].datasetPathname + markersConfigDataDirCLen, buf);
            markersNFT[i].pageNo = -1;
        } else {
            ARLOGe("Error in marker configuration data file; unsupported marker type %s.\n", buf1);
        }
        
        // Look for optional tokens. A blank line marks end of options.
        while (get_buff(buf, MAXPATHLEN, fp, 0) && (buf[0] != '\0')) {
            if (strncmp(buf, "FILTER", 6) == 0) {
                markersNFT[i].filterCutoffFrequency = AR_FILTER_TRANS_MAT_CUTOFF_FREQ_DEFAULT;
                markersNFT[i].filterSampleRate = AR_FILTER_TRANS_MAT_SAMPLE_RATE_DEFAULT;
                if (strlen(buf) != 6) {
                    if (sscanf(&buf[6],
#ifdef ARDOUBLE_IS_FLOAT
                               "%f"
#else
                               "%lf"
#endif
                               , &tempF) == 1) markersNFT[i].filterCutoffFrequency = tempF;
                }
                markersNFT[i].ftmi = arFilterTransMatInit(markersNFT[i].filterSampleRate, markersNFT[i].filterCutoffFrequency);
            }
            // Unknown tokens are ignored.
        }
    }
    fclose(fp);
    
    // If not all markers were read, an error occurred.
    if (i < markersNFTCount) {
    
        // Clean up.
        for (; i >= 0; i--) {
            if (markersNFT[i].datasetPathname)  free(markersNFT[i].datasetPathname);
            if (markersNFT[i].ftmi) arFilterTransMatFinal(markersNFT[i].ftmi);
        }
        free(markersNFT);
                
        *markersNFTCount_out = 0;
        *markersNFT_out = NULL;
        return;
    }
    
    *markersNFTCount_out = markersNFTCount;
    *markersNFT_out = markersNFT;
}

void deleteMarkers(ARMarkerNFT **markersNFT_p, int *markersNFTCount_p)
{
    int i;
    
    if (!markersNFT_p || !*markersNFT_p || !*markersNFTCount_p || *markersNFTCount_p < 1) return;
    
    for (i = 0; i < *markersNFTCount_p; i++) {
        if ((*markersNFT_p)[i].datasetPathname) {
            free((*markersNFT_p)[i].datasetPathname);
            (*markersNFT_p)[i].datasetPathname = NULL;
        }
        if ((*markersNFT_p)[i].ftmi) {
            arFilterTransMatFinal((*markersNFT_p)[i].ftmi);
            (*markersNFT_p)[i].ftmi = NULL;
        }
    }
    free(*markersNFT_p);
    *markersNFT_p = NULL;
    *markersNFTCount_p = 0;
}

#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))

void color_convert_common(unsigned char *pY, unsigned char *pUV, int width, int height, unsigned char *buffer)
{
	const int bytes_per_pixel = 2;

	int nR, nG, nB, nY, nU, nV, i, j, id2, jd2, offset;
	unsigned char *out = buffer;

	offset = 0;

	// YUV 4:2:0
	for (i = 0; i < height; i++) {

		id2 = i >> 1; // Divide by two

	    for (j = 0; j < width; j++) {

			jd2 = j >> 1; // Divide by two

			nY = *(pY + i * width + j);
			nV = *(pUV + id2 * width + bytes_per_pixel * jd2);
			nU = *(pUV + id2 * width + bytes_per_pixel * jd2 + 1);

			// Yuv Convert
			nY = MAX(nY - 16, 0);
			nU -= 128;
			nV -= 128;

			nB = 1192 * nY + 2066 * nU;
			nG = 1192 * nY - 833 * nV - 400 * nU;
			nR = 1192 * nY + 1634 * nV;

			nR = MIN(262143, MAX(0, nR));
			nG = MIN(262143, MAX(0, nG));
			nB = MIN(262143, MAX(0, nB));

			nR >>= 10; nR &= 0xff;
			nG >>= 10; nG &= 0xff;
			nB >>= 10; nB &= 0xff;

			out[offset++] = (unsigned char)nR;
			out[offset++] = (unsigned char)nG;
			out[offset++] = (unsigned char)nB;
			out[offset++] = (unsigned char)255;
	    }
	}
}

