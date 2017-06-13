/*
 *  nftBook.cpp
 *  ARToolKit for Android
 *
 *  An NFT example with all ARToolKit setup performed in native code,
 *  and with OpenSceneGraph rendering of models.
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

// ============================================================================
//	Includes
// ============================================================================

#include <jni.h>
#include <android/log.h>
#include <stdlib.h> // malloc()

#include <AR/ar.h>
#include <AR/arMulti.h>
#include <AR/video.h>
#include <AR/gsub_es.h>
#include <AR/arFilterTransMat.h>
#include <AR2/tracking.h>
#include <AR/arosg.h>

#include "ARMarkerNFT.h"
#include "trackingSub.h"
#include "VirtualEnvironment.h"
#include "osgPlugins.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>


//#include <android/log.h>

//#define  LOG_TAG    "FRAME TAG"

//#define  LOGX(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
//#define  LOGY(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
//	Types
// ============================================================================

typedef enum {
    ARViewContentModeScaleToFill,
    ARViewContentModeScaleAspectFit,      // contents scaled to fit with fixed aspect. remainder is transparent
    ARViewContentModeScaleAspectFill,     // contents scaled to fill with fixed aspect. some portion of content may be clipped.
    //ARViewContentModeRedraw,              // redraw on bounds change
    ARViewContentModeCenter,              // contents remain same size. positioned adjusted.
    ARViewContentModeTop,
    ARViewContentModeBottom,
    ARViewContentModeLeft,
    ARViewContentModeRight,
    ARViewContentModeTopLeft,
    ARViewContentModeTopRight,
    ARViewContentModeBottomLeft,
    ARViewContentModeBottomRight,
} ARViewContentMode;

enum viewPortIndices {
    viewPortIndexLeft = 0,
    viewPortIndexBottom,
    viewPortIndexWidth,
    viewPortIndexHeight
};

// ============================================================================
//	Constants
// ============================================================================

#define PAGES_MAX               10          // Maximum number of pages expected. You can change this down (to save memory) or up (to accomodate more pages.)

#ifndef MAX
#  define MAX(X,Y) ((X) > (Y) ? (X) : (Y))
#endif
#ifndef MIN
#  define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#endif

// Logging macros
#define  LOG_TAG    "nftBookNative"
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

// ============================================================================
//	Function prototypes.
// ============================================================================

// Utility preprocessor directive so only one change needed if Java class name changes
#define JNIFUNCTION_NATIVE(sig) Java_org_artoolkit_ar_samples_nftBook_nftBookActivity_##sig

extern "C" {
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeCreate(JNIEnv* env, jobject object, jobject instanceOfAndroidContext));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStart(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStop(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeDestroy(JNIEnv* env, jobject object));
	JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeVideoInit(JNIEnv* env, jobject object, jint w, jint h, jint cameraIndex, jboolean cameraIsFrontFacing));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeVideoFrame(JNIEnv* env, jobject obj, jshort frame_id_need_update, jbyteArray pinArray, jbyteArray jpegArray)) ;
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceCreated(JNIEnv* env, jobject object));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDisplayParametersChanged(JNIEnv* env, jobject object, jint orientation, jint width, jint height, jint dpi));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj));
	JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSetInternetState(JNIEnv* env, jobject obj, jint state));
};

static void nativeVideoGetCparamCallback(const ARParam *cparam, void *userdata);
static void *loadNFTDataAsync(THREAD_HANDLE_T *threadHandle);
static int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat);
static void *receive_marker_handler(void* n);

// ============================================================================
//	Global variables
// ============================================================================

// Preferences.
static const char *cparaName = "Data/camera_para.dat";				///< Camera parameters file
static const char *markerConfigDataFilename = "Data/markers.dat";
static const char *objectDataFilename = "Data/objects.dat";

// Image acquisition.
static AR2VideoParamT *gVid = NULL;
static bool videoInited = false;                                    ///< true when ready to receive video frames.
static int videoWidth = 0;                                          ///< Width of the video frame in pixels.
static int videoHeight = 0;                                         ///< Height of the video frame in pixels.
static AR_PIXEL_FORMAT gPixFormat;                                  ///< Pixel format from ARToolKit enumeration.
static ARUint8* gVideoFrame = NULL;                                 ///< Buffer containing current video frame.
static size_t gVideoFrameSize = 0;                                  ///< Size of buffer containing current video frame.
static ARUint8 *myRGBABuffer = NULL;
static ARUint8 *myJPEGBuffer = NULL;
static size_t myRGBABufferSize = 0;
static size_t myJPEGBufferSize = 0;
static bool videoFrameNeedsPixelBufferDataUpload = false;
static int gCameraIndex = 0;
static bool gCameraIsFrontFacing = false;

// Markers.
static ARMarkerNFT *markersNFT = NULL;
static int markersNFTCount = 0;

// NFT.
static THREAD_HANDLE_T     *trackingThreadHandle = NULL;
static AR2HandleT          *ar2Handle = NULL;
static KpmHandle           *kpmHandle = NULL;
static int                  surfaceSetCount = 0;
static AR2SurfaceSetT      *surfaceSet[PAGES_MAX];
static THREAD_HANDLE_T     *nftDataLoadingThreadHandle = NULL;
static int                  nftDataLoaded = false;

// NFT results.
static int detectedPage = -2; // -2 Tracking not inited, -1 tracking inited OK, >= 0 tracking online on page.
static float trackingTrans[3][4];

// Drawing.
static int backingWidth;
static int backingHeight;
static GLint viewPort[4];
static ARViewContentMode gContentMode = ARViewContentModeScaleAspectFill;
static bool gARViewLayoutRequired = false;
static ARParamLT *gCparamLT = NULL;                                 ///< Camera paramaters
static ARGL_CONTEXT_SETTINGS_REF gArglSettings = NULL;              ///< GL settings for rendering video background
static const ARdouble NEAR_PLANE = 10.0f;                           ///< Near plane distance for projection matrix calculation
static const ARdouble FAR_PLANE = 5000.0f;                          ///< Far plane distance for projection matrix calculation
static ARdouble cameraLens[16];
static ARdouble cameraPose[16];
static int cameraPoseValid;
static bool gARViewInited = false;

// Drawing orientation.
static int gDisplayOrientation = 1; // range [0-3]. 1=landscape.
static int gDisplayWidth = 0;
static int gDisplayHeight = 0;
static int gDisplayDPI = 160; // Android default.

static bool gContentRotate90 = false;
static bool gContentFlipV = false;
static bool gContentFlipH = false;

// Network.
static int gInternetState = -1;

// sender client socket
struct sockaddr_in myaddr;  /* our address */
struct sockaddr_in remaddr; /* remote address */
struct sockaddr_in dstaddr; /* destination address */
socklen_t addrlen = sizeof(myaddr);    /* length of addresses */
socklen_t remaddrlen = sizeof(remaddr);
socklen_t dstaddrlen = sizeof(dstaddr);
int recvlen;      /* # bytes received */
int fd;       /* our socket */
int send_fd;
int SERVICE_PORT = 10000;
int marker_buf_size = 2880;
static bool thread_already_running = false;
static bool sender_thread_already_running = false;

static short frame_id_update = 0;
static short frame_id = 0;


// ============================================================================
//	Functions
// ============================================================================

//
// Lifecycle functions.
// See http://developer.android.com/reference/android/app/Activity.html#ActivityLifecycle
//

void *receive_marker_handler(void* thread_id)
{
    while (true)
    {
        unsigned char buf[marker_buf_size];
        int recvlen = recvfrom(fd, buf, marker_buf_size, 0, (struct sockaddr *)&remaddr, &remaddrlen);
        buf[recvlen]='\0';

        memcpy(&detectedPage, buf, 4);

        int buf_ptr = 4;

        for (int i = 0; i < 3; i ++)
        {
            for (int j = 0; j < 4; j ++)
            {
                memcpy(&trackingTrans[i][j], buf + buf_ptr, 4);
                buf_ptr += 4;
            }
        }

        LOGE("recved are %d, %f, %f, %f\n", detectedPage, trackingTrans[0][0], trackingTrans[0][2], trackingTrans[2][0]);
    }
}

void *send_RGB_frame_handler(void* thread_id)
{
    while (true)
    // while (false)
    {
        if (frame_id_update == 0)
            frame_id = 0;

        // LOGD("Zhaowei: Current frame_id is %d, frame_id_update is %d", frame_id, frame_id_update);

        // LOGD("Zhaowei: indicator is %d", new_frame_indicator);
        if (frame_id <= frame_id_update) {
            LOGD("Zhaowei: JPEG sent. Size is %d, frame_id is %d.", myJPEGBufferSize, frame_id);

            int sent_buffer_size = 0;
            short segment_id = 0;
            // while (sent_buffer_size < myRGBABufferSize) {
            while (sent_buffer_size < myJPEGBufferSize) {
                int length_to_send = 500;
                short last_segment_tag = 0;
                // if (sent_buffer_size + length_to_send > myRGBABufferSize) {
                if (sent_buffer_size + length_to_send > myJPEGBufferSize) {
                    // length_to_send = myRGBABufferSize - sent_buffer_size;
                    length_to_send = myJPEGBufferSize - sent_buffer_size;
                    LOGD("set last segment tag\n");
                    last_segment_tag = 1;
                }


                // generate header
                char* full_message = (char*)malloc(6+length_to_send);
                memcpy(full_message, &frame_id, 2);
                memcpy(full_message+2, &segment_id, 2);
                memcpy(full_message+4, &last_segment_tag, 2);

                // copy data
                // memcpy(full_message+6, myRGBABuffer+sent_buffer_size, length_to_send);
                memcpy(full_message+6, myJPEGBuffer+sent_buffer_size, length_to_send);


                if (sendto(send_fd, full_message, 6+length_to_send, 0, (struct sockaddr *)&dstaddr, sizeof(dstaddr)) < 0)
                {
                    LOGE("Sending frame to server failed.\n");
                }

                sent_buffer_size += length_to_send;
                segment_id ++;
            }

            //free(last_segment_tag);
            frame_id ++;
        }
    }

}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeCreate(JNIEnv* env, jobject object, jobject instanceOfAndroidContext))
{
    int err_i;
#ifdef DEBUG
    LOGI("nativeCreate\n");
#endif

    // Change working directory for the native process, so relative paths can be used for file access.
    arUtilChangeToResourcesDirectory(AR_UTIL_RESOURCES_DIRECTORY_BEHAVIOR_BEST, NULL, instanceOfAndroidContext);

    // Load marker(s).
    newMarkers(markerConfigDataFilename, &markersNFT, &markersNFTCount);
    if (!markersNFTCount) {
        LOGE("Error loading markers from config. file '%s'.", markerConfigDataFilename);
        return false;
    }
#ifdef DEBUG
    LOGE("Marker count = %d\n", markersNFTCount);
#endif

    /* receiving UDP packets from receiver client */
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOGE("cannot create socket\n");
        return false;
    }

    /* bind the socket to any valid IP address and a specific port */
    memset((char *)&myaddr, 0, sizeof(myaddr));
    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    myaddr.sin_port = htons(10001);

    if (bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
        LOGE("bind failed\n");
        return false;
    }
    else
        LOGE("bind success\n");

    /* sending UDP packets to server client */
    if ((send_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOGE("cannot create socket\n");
        return false;
    }

    memset((char *)&dstaddr, 0, sizeof(dstaddr));
    dstaddr.sin_family = AF_INET;
    dstaddr.sin_addr.s_addr = inet_addr("192.168.2.1");
    //dstaddr.sin_addr.s_addr = inet_addr("131.179.210.70");
    dstaddr.sin_port = htons(10000);

	return (true);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStart(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeStart\n");
#endif

    gVid = ar2VideoOpen("");
    if (!gVid) {
    	LOGE("Error: ar2VideoOpen.\n");
    	return (false);
    }

    // Since most NFT init can't be completed until the video frame size is known,
    // and NFT surface loading depends on NFT init, all that will be deferred.
    
    // Also, VirtualEnvironment init depends on having an OpenGL context, and so that also 
    // forces us to defer VirtualEnvironment init.
    
    // ARGL init depends on both these things, which forces us to defer it until the
    // main frame loop.
        
	return (true);
}

// cleanup();
JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeStop(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeStop\n");
#endif
    int i, j;

	// Can't call arglCleanup() or VirtualEnvironmentFinal() here, because nativeStop is not called on rendering thread.

    // NFT cleanup.
    if (trackingThreadHandle) {
#ifdef DEBUG
        LOGI("Stopping NFT2 tracking thread.");
#endif
        trackingInitQuit(&trackingThreadHandle);
        detectedPage = -2;
    }
    j = 0;
    for (i = 0; i < surfaceSetCount; i++) {
        if (surfaceSet[i]) {
#ifdef DEBUG
            if (j == 0) LOGI("Unloading NFT tracking surfaces.");
#endif
            ar2FreeSurfaceSet(&surfaceSet[i]); // Sets surfaceSet[i] to NULL.
            j++;
        }
    }
#ifdef DEBUG
    if (j > 0) LOGI("Unloaded %d NFT tracking surfaces.", j);
#endif
    surfaceSetCount = 0;
    nftDataLoaded = false;
#ifdef DEBUG
	LOGI("Cleaning up ARToolKit NFT handles.");
#endif
    ar2DeleteHandle(&ar2Handle);
    kpmDeleteHandle(&kpmHandle);
    arParamLTFree(&gCparamLT);

    // OpenGL cleanup -- not done here.

    // Video cleanup.
    if (gVideoFrame) {
        free(gVideoFrame);
        gVideoFrame = NULL;
        gVideoFrameSize = 0;
    }
    ar2VideoClose(gVid);
    gVid = NULL;
    videoInited = false;

    return (true);
}

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeDestroy(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeDestroy\n");
#endif
    if (markersNFT) deleteMarkers(&markersNFT, &markersNFTCount);
    
    return (true);
}

//
// Camera functions.
//

JNIEXPORT jboolean JNICALL JNIFUNCTION_NATIVE(nativeVideoInit(JNIEnv* env, jobject object, jint w, jint h, jint cameraIndex, jboolean cameraIsFrontFacing))
{
#ifdef DEBUG
    LOGI("nativeVideoInit\n");
#endif
	// As of ARToolKit v5.0, NV21 format video frames are handled natively,
	// and no longer require colour conversion to RGBA. A buffer (gVideoFrame)
	// must be set aside to copy the frame from the Java side.
	// If you still require RGBA format information from the video,
	// you can create your own additional buffer, and then unpack the NV21
	// frames into it in nativeVideoFrame() below.
	// Here is where you'd allocate the buffer:
	myRGBABufferSize = w * h * 4;
	myRGBABuffer = (ARUint8 *)malloc(myRGBABufferSize);

	gPixFormat = AR_PIXEL_FORMAT_NV21;
	gVideoFrameSize = (sizeof(ARUint8)*(w*h + 2*w/2*h/2));
	gVideoFrame = (ARUint8*) (malloc(gVideoFrameSize));
	if (!gVideoFrame) {
		gVideoFrameSize = 0;
		LOGE("Error allocating frame buffer");
		return false;
	}
	videoWidth = w;
	videoHeight = h;
	gCameraIndex = cameraIndex;
	gCameraIsFrontFacing = cameraIsFrontFacing;
	LOGI("Video camera %d (%s), %dx%d format %s, %d-byte buffer.", gCameraIndex, (gCameraIsFrontFacing ? "front" : "rear"), w, h, arUtilGetPixelFormatName(gPixFormat), gVideoFrameSize);

	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_WIDTH, videoWidth);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_HEIGHT, videoHeight);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_PIXELFORMAT, (int)gPixFormat);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_CAMERA_INDEX, gCameraIndex);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_CAMERA_FACE, gCameraIsFrontFacing);
	ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_INTERNET_STATE, gInternetState);

	if (ar2VideoGetCParamAsync(gVid, nativeVideoGetCparamCallback, NULL) < 0) {
		LOGE("Error getting cparam.\n");
		nativeVideoGetCparamCallback(NULL, NULL);
	}

	return (true);
}

static void nativeVideoGetCparamCallback(const ARParam *cparam_p, void *userdata)
{
	// Load the camera parameters, resize for the window and init.
	ARParam cparam;
	if (cparam_p) cparam = *cparam_p;
	else {
	    LOGE("Unable to automatically determine camera parameters. Using default.\n");
        if (arParamLoad(cparaName, 1, &cparam) < 0) {
            LOGE("Error: Unable to load parameter file %s for camera.\n", cparaName);
            return;
        }
	}
	if (cparam.xsize != videoWidth || cparam.ysize != videoHeight) {
		LOGW("*** Camera Parameter resized from %d, %d. ***\n", cparam.xsize, cparam.ysize);
		arParamChangeSize(&cparam, videoWidth, videoHeight, &cparam);
	}
#ifdef DEBUG
	LOGD("*** Camera Parameter ***\n");
	arParamDisp(&cparam);
#endif
	if ((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL) {
		LOGE("Error: arParamLTCreate.\n");
		return;
	}
	videoInited = true;

	//
	// AR init.
	//
    
	// Create the OpenGL projection from the calibrated camera parameters.
	arglCameraFrustumRHf(&gCparamLT->param, NEAR_PLANE, FAR_PLANE, cameraLens);
	cameraPoseValid = FALSE;

	if (!initNFT(gCparamLT, gPixFormat)) {
		LOGE("Error initialising NFT.\n");
		arParamLTFree(&gCparamLT);
		return;
	}

	// Marker data has already been loaded, so now load NFT data on a second thread.
	nftDataLoadingThreadHandle = threadInit(0, NULL, loadNFTDataAsync);
	if (!nftDataLoadingThreadHandle) {
		LOGE("Error starting NFT loading thread.\n");
		arParamLTFree(&gCparamLT);
		return;
	}
	threadStartSignal(nftDataLoadingThreadHandle);

}

// Modifies globals: kpmHandle, ar2Handle.
static int initNFT(ARParamLT *cparamLT, AR_PIXEL_FORMAT pixFormat)
{
#ifdef DEBUG
    LOGE("Initialising NFT.\n");
#endif
    //
    // NFT init.
    //
    
    // KPM init.
    kpmHandle = kpmCreateHandle(cparamLT, pixFormat);
    if (!kpmHandle) {
        LOGE("Error: kpmCreatHandle.\n");
        return (false);
    }
    //kpmSetProcMode( kpmHandle, KpmProcHalfSize );
    
    // AR2 init.
    if( (ar2Handle = ar2CreateHandle(cparamLT, pixFormat, AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL ) {
        LOGE("Error: ar2CreateHandle.\n");
        kpmDeleteHandle(&kpmHandle);
        return (false);
    }
    if (threadGetCPU() <= 1) {
#ifdef DEBUG
        LOGE("Using NFT tracking settings for a single CPU.\n");
#endif
        ar2SetTrackingThresh( ar2Handle, 5.0 );
        ar2SetSimThresh( ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 6);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    } else {
#ifdef DEBUG
        LOGE("Using NFT tracking settings for more than one CPU.\n");
#endif
        ar2SetTrackingThresh( ar2Handle, 5.0 );
        ar2SetSimThresh( ar2Handle, 0.50 );
        ar2SetSearchFeatureNum(ar2Handle, 16);
        ar2SetSearchSize(ar2Handle, 12);
        ar2SetTemplateSize1(ar2Handle, 6);
        ar2SetTemplateSize2(ar2Handle, 6);
    }
    // NFT dataset loading will happen later.
#ifdef DEBUG
    LOGE("NFT initialised OK.\n");
#endif
    return (true);
}

// References globals: markersNFTCount
// Modifies globals: trackingThreadHandle, surfaceSet[], surfaceSetCount, markersNFT[], markersNFTCount
static void *loadNFTDataAsync(THREAD_HANDLE_T *threadHandle)
{
    int i, j;
	KpmRefDataSet *refDataSet;
    
    while (threadStartWait(threadHandle) == 0) {
#ifdef DEBUG
        LOGE("Loading NFT data.\n");
#endif
    
        // If data was already loaded, stop KPM tracking thread and unload previously loaded data.
        if (trackingThreadHandle) {
            LOGE("NFT2 tracking thread is running. Stopping it first.\n");
            trackingInitQuit(&trackingThreadHandle);
            detectedPage = -2;
        }
        j = 0;
        for (i = 0; i < surfaceSetCount; i++) {
            if (j == 0) LOGE("Unloading NFT tracking surfaces.");
            ar2FreeSurfaceSet(&surfaceSet[i]); // Also sets surfaceSet[i] to NULL.
            j++;
        }
        if (j > 0) LOGE("Unloaded %d NFT tracking surfaces.\n", j);
        surfaceSetCount = 0;
    
        refDataSet = NULL;
    
        for (i = 0; i < markersNFTCount; i++) {
            // Load KPM data.
            KpmRefDataSet  *refDataSet2;
            LOGI("Reading %s.fset3\n", markersNFT[i].datasetPathname);
            if (kpmLoadRefDataSet(markersNFT[i].datasetPathname, "fset3", &refDataSet2) < 0 ) {
                LOGE("Error reading KPM data from %s.fset3\n", markersNFT[i].datasetPathname);
                markersNFT[i].pageNo = -1;
                continue;
            }
            markersNFT[i].pageNo = surfaceSetCount;
            LOGI("  Assigned page no. %d.\n", surfaceSetCount);
            if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, surfaceSetCount) < 0) {
                LOGE("Error: kpmChangePageNoOfRefDataSet\n");
                exit(-1);
            }
            if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
                LOGE("Error: kpmMergeRefDataSet\n");
                exit(-1);
            }
            LOGI("  Done.\n");
        
            // Load AR2 data.
            LOGI("Reading %s.fset\n", markersNFT[i].datasetPathname);

            if ((surfaceSet[surfaceSetCount] = ar2ReadSurfaceSet(markersNFT[i].datasetPathname, "fset", NULL)) == NULL ) {
                LOGE("Error reading data from %s.fset\n", markersNFT[i].datasetPathname);
            }
            LOGI("  Done.\n");
        
            surfaceSetCount++;
            if (surfaceSetCount == PAGES_MAX) break;
        }
        if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0) {
            LOGE("Error: kpmSetRefDataSet");
            exit(-1);
        }
        kpmDeleteRefDataSet(&refDataSet);
    
        // Start the KPM tracking thread.
        trackingThreadHandle = trackingInitInit(kpmHandle);
        if (!trackingThreadHandle) exit(-1);

#ifdef DEBUG
        LOGI("Loading of NFT data complete.");
#endif

        threadEndSignal(threadHandle); // Signal that we're done.
    }
    return (NULL); // Exit this thread.
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeVideoFrame(JNIEnv* env, jobject obj, jshort frame_id_need_update, jbyteArray pinArray, jbyteArray jpegArray))
{

    int i, j, k;
    jbyte* inArray;
    frame_id_update = (short)frame_id_need_update;

    if (!videoInited) {
#ifdef DEBUG
        LOGD("nativeVideoFrame !VIDEO\n");
#endif
        return; // No point in trying to track until video is inited.
    }
    if (!nftDataLoaded) {
        if (!nftDataLoadingThreadHandle || threadGetStatus(nftDataLoadingThreadHandle) < 1) {
#ifdef DEBUG
            LOGD("nativeVideoFrame !NFTDATA\n");
#endif
            return;
        } else {
            nftDataLoaded = true;
            threadWaitQuit(nftDataLoadingThreadHandle);
            threadFree(&nftDataLoadingThreadHandle); // Clean up.
        }
    }
    if (!gARViewInited) {
        return; // Also, we won't track until the ARView has been inited.
#ifdef DEBUG
        LOGD("nativeVideoFrame !ARVIEW\n");
#endif
    }
#ifdef DEBUG
    LOGD("nativeVideoFrame\n");
#endif

    // Copy the incoming  YUV420 image in pinArray.
    env->GetByteArrayRegion(pinArray, 0, gVideoFrameSize, (jbyte *)gVideoFrame);

    // Copy the jpeg in jpegArray
    myJPEGBufferSize = env->GetArrayLength (jpegArray);
    myJPEGBuffer = (ARUint8 *)malloc(myJPEGBufferSize);
    LOGD("Zhaowei: new JPEG id: %d, size %d", frame_id, myJPEGBufferSize);
    env->GetByteArrayRegion (jpegArray, 0, myJPEGBufferSize, (jbyte *) myJPEGBuffer);

    if (frame_id_update == 0)
                frame_id = 0;

    // LOGD("Zhaowei: Current frame_id is %d, frame_id_update is %d", frame_id, frame_id_update);

    // LOGD("Zhaowei: indicator is %d", new_frame_indicator);
    LOGD("Zhaowei: JPEG sent. Size is %d, frame_id is %d.", myJPEGBufferSize, frame_id);

    int sent_buffer_size = 0;
    short segment_id = 0;
    // while (sent_buffer_size < myRGBABufferSize) {
    while (sent_buffer_size < myJPEGBufferSize) {
        int length_to_send = 1000;
        short last_segment_tag = 0;
        // if (sent_buffer_size + length_to_send > myRGBABufferSize) {
        if (sent_buffer_size + length_to_send > myJPEGBufferSize) {
            // length_to_send = myRGBABufferSize - sent_buffer_size;
            length_to_send = myJPEGBufferSize - sent_buffer_size;
            LOGD("set last segment tag\n");
            last_segment_tag = 1;
        }


        // generate header
        char* full_message = (char*)malloc(6+length_to_send);
        memcpy(full_message, &frame_id, 2);
        memcpy(full_message+2, &segment_id, 2);
        memcpy(full_message+4, &last_segment_tag, 2);

        // copy data
        // memcpy(full_message+6, myRGBABuffer+sent_buffer_size, length_to_send);
        memcpy(full_message+6, myJPEGBuffer+sent_buffer_size, length_to_send);


        if (sendto(send_fd, full_message, 6+length_to_send, 0, (struct sockaddr *)&dstaddr, sizeof(dstaddr)) < 0)
        {
            LOGE("Sending frame to server failed.\n");
        }

        sent_buffer_size += length_to_send;
        segment_id ++;
    }

    //free(last_segment_tag);
    frame_id ++;

	// As of ARToolKit v5.0, NV21 format video frames are handled natively,
	// and no longer require colour conversion to RGBA.
	// If you still require RGBA format information from the video,
    // here is where you'd do the conversion:

    // color_convert_common(gVideoFrame, gVideoFrame + videoWidth*videoHeight, videoWidth, videoHeight, myRGBABuffer);


    /*pthread_t sender_thread;

    if (!sender_thread_already_running)
    {
        int* tid = (int*)malloc(sizeof(int));
        if(pthread_create(&sender_thread, NULL, send_RGB_frame_handler, (void*)tid) < 0)
        {
            LOGE("could not create thread\n");
        }
        else
        {
            sender_thread_already_running = true;
            LOGE("sender thread is running\n");
        }
    }*/



    videoFrameNeedsPixelBufferDataUpload = true; // Note that buffer needs uploading. (Upload must be done on OpenGL context's thread.)

    // Run marker detection on frame
    /*if (trackingThreadHandle) {
        // Perform NFT tracking.
        float            err;
        int              ret;
        int              pageNo;

        if( detectedPage == -2 ) {
            trackingInitStart( trackingThreadHandle, gVideoFrame );
            detectedPage = -1;
        }
        if( detectedPage == -1 ) {
            ret = trackingInitGetResult( trackingThreadHandle, trackingTrans, &pageNo);
            if( ret == 1 ) {
                if (pageNo >= 0 && pageNo < surfaceSetCount) {
#ifdef DEBUG
                    LOGE("Detected page %d.\n", pageNo);
#endif
                    detectedPage = pageNo;
                    ar2SetInitTrans(surfaceSet[detectedPage], trackingTrans);
                } else {
                    LOGE("Detected bad page %d.\n", pageNo);
                    detectedPage = -2;
                }
            } else if( ret < 0 ) {
#ifdef DEBUG
                LOGE("No page detected.\n");
#endif
                detectedPage = -2;
            }
        }
        if( detectedPage >= 0 && detectedPage < surfaceSetCount) {
            if( ar2Tracking(ar2Handle, surfaceSet[detectedPage], gVideoFrame, trackingTrans, &err) < 0 ) {
#ifdef DEBUG
                LOGE("Tracking lost.\n");
#endif
                detectedPage = -2;
            } else {
#ifdef DEBUG
                LOGE("Tracked page %d (max %d).\n", detectedPage, surfaceSetCount - 1);
#endif
            }
        }
    } else {
        LOGE("Error: trackingThreadHandle\n");
        detectedPage = -2;
    }*/

    pthread_t receiver_thread;

    if (!thread_already_running)
    {
        int* tid = (int*)malloc(sizeof(int));
        if(pthread_create(&receiver_thread, NULL, receive_marker_handler, (void*)tid) < 0)
        {
            LOGE("could not create thread\n");
        }
        else
        {
            thread_already_running = true;
            LOGE("thread is running\n");
        }
    }

    // Update markers.
    for (i = 0; i < markersNFTCount; i++) {
        markersNFT[i].validPrev = markersNFT[i].valid;
        if (markersNFT[i].pageNo >= 0 && markersNFT[i].pageNo == detectedPage) {
            markersNFT[i].valid = TRUE;
            for (j = 0; j < 3; j++) for (k = 0; k < 4; k++) markersNFT[i].trans[j][k] = trackingTrans[j][k];
        }
        else markersNFT[i].valid = FALSE;
        if (markersNFT[i].valid) {

            // Filter the pose estimate.
            if (markersNFT[i].ftmi) {
                if (arFilterTransMat(markersNFT[i].ftmi, markersNFT[i].trans, !markersNFT[i].validPrev) < 0) {
                    LOGE("arFilterTransMat error with marker %d.\n", i);
                }
            }

            if (!markersNFT[i].validPrev) {
                // Marker has become visible, tell any dependent objects.
                VirtualEnvironmentHandleARMarkerAppeared(i);
            }

            // We have a new pose, so set that.
            arglCameraViewRHf(markersNFT[i].trans, markersNFT[i].pose.T, 1.0f /*VIEW_SCALEFACTOR*/);
            // Tell any dependent objects about the update.
            VirtualEnvironmentHandleARMarkerWasUpdated(i, markersNFT[i].pose);

        } else {

            if (markersNFT[i].validPrev) {
                // Marker has ceased to be visible, tell any dependent objects.
                VirtualEnvironmentHandleARMarkerDisappeared(i);
            }
        }
    }
}

//
// OpenGL functions.
//

//
// This is called whenever the OpenGL context has just been created or re-created.
// Note that GLSurfaceView is a bit asymmetrical here; we don't get a call when the
// OpenGL context is about to be deleted, it's just whipped out from under us. So it's
// possible that when we enter this function, we're actually resuming after such an
// event. What about resources we allocated previously which we didn't get time to
// de-allocate? Well, we don't have to worry about the OpenGL resources themselves, they
// were deleted along with the context. But, we should clean up any data structures we
// allocated with malloc etc. ARGL's settings falls into this category.
//
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceCreated(JNIEnv* env, jobject object))
{
#ifdef DEBUG
    LOGI("nativeSurfaceCreated\n");
#endif        
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	glStateCacheFlush(); // Make sure we don't hold outdated OpenGL state.

	if (gArglSettings) {
		arglCleanup(gArglSettings); // Clean up any left-over ARGL data.
		gArglSettings = NULL;
	}
	VirtualEnvironmentFinal(); // Clean up any left-over OSG data.
	
    gARViewInited = false;
}

//
// This is called when something about the surface changes. e.g. size.
//
// Modifies globals: backingWidth, backingHeight, gARViewLayoutRequired.
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSurfaceChanged(JNIEnv* env, jobject object, jint w, jint h))
{
    backingWidth = w;
    backingHeight = h;
#ifdef DEBUG
    LOGI("nativeSurfaceChanged backingWidth=%d, backingHeight=%d\n", w, h);
#endif        
    
	// Call through to anyone else who needs to know about window sizing here.

    // In order to do something meaningful with the surface backing size in an AR sense,
    // we also need the content size, which we aren't guaranteed to have yet, so defer
    // the viewPort calculations.
    gARViewLayoutRequired = true;
}

// 0 = portrait, 1 = landscape (device rotated 90 degrees ccw), 2 = portrait upside down, 3 = landscape reverse (device rotated 90 degrees cw).
JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDisplayParametersChanged(JNIEnv* env, jobject object, jint orientation, jint width, jint height, jint dpi))
{
#ifdef DEBUG
    LOGI("nativeDisplayParametersChanged orientation=%d, size=%dx%d@%dpi\n", orientation, width, height, dpi);
#endif
	gDisplayOrientation = orientation;
	gDisplayWidth = width;
	gDisplayHeight = height;
	gDisplayDPI = dpi;

    gARViewLayoutRequired = true;
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeSetInternetState(JNIEnv* env, jobject obj, jint state))
{
	gInternetState = state;
	if (gVid) {
		ar2VideoSetParami(gVid, AR_VIDEO_PARAM_ANDROID_INTERNET_STATE, state);
	}
}

// Lays out the AR view. Requires both video and OpenGL to be inited, and must be called on OpenGL thread.
// References globals: gContentMode, backingWidth, backingHeight, videoWidth, videoHeight, .
// Modifies globals: gContentFlipV, gContentFlipH, gContentRotate90, viewPort, gARViewLayoutRequired.
static bool layoutARView(void)
{
	if (gDisplayOrientation == 0) {
		gContentRotate90 = true;
		gContentFlipV = false;
		gContentFlipH = gCameraIsFrontFacing;
	} else if (gDisplayOrientation == 1) {
		gContentRotate90 = false;
		gContentFlipV = false;
		gContentFlipH = gCameraIsFrontFacing;
	} else if (gDisplayOrientation == 2) {
		gContentRotate90 = true;
		gContentFlipV = true;
		gContentFlipH = (!gCameraIsFrontFacing);
	} else if (gDisplayOrientation == 3) {
		gContentRotate90 = false;
		gContentFlipV = true;
		gContentFlipH = (!gCameraIsFrontFacing);
	}
    arglSetRotate90(gArglSettings, gContentRotate90);
    arglSetFlipV(gArglSettings, gContentFlipV);
    arglSetFlipH(gArglSettings, gContentFlipH);

    // Calculate viewPort.
    int left, bottom, w, h;
    int contentWidth = videoWidth;
    int contentHeight = videoHeight;

	if (gContentMode == ARViewContentModeScaleToFill) {
        w = backingWidth;
        h = backingHeight;
    } else {
        int contentWidthFinalOrientation = (gContentRotate90 ? contentHeight : contentWidth);
        int contentHeightFinalOrientation = (gContentRotate90 ? contentWidth : contentHeight);
        if (gContentMode == ARViewContentModeScaleAspectFit || gContentMode == ARViewContentModeScaleAspectFill) {
            float scaleRatioWidth, scaleRatioHeight, scaleRatio;
            scaleRatioWidth = (float)backingWidth / (float)contentWidthFinalOrientation;
            scaleRatioHeight = (float)backingHeight / (float)contentHeightFinalOrientation;
            if (gContentMode == ARViewContentModeScaleAspectFill) scaleRatio = MAX(scaleRatioHeight, scaleRatioWidth);
            else scaleRatio = MIN(scaleRatioHeight, scaleRatioWidth);
            w = (int)((float)contentWidthFinalOrientation * scaleRatio);
            h = (int)((float)contentHeightFinalOrientation * scaleRatio);
        } else {
            w = contentWidthFinalOrientation;
            h = contentHeightFinalOrientation;
        }
    }
    
    if (gContentMode == ARViewContentModeTopLeft
        || gContentMode == ARViewContentModeLeft
        || gContentMode == ARViewContentModeBottomLeft) left = 0;
    else if (gContentMode == ARViewContentModeTopRight
             || gContentMode == ARViewContentModeRight
             || gContentMode == ARViewContentModeBottomRight) left = backingWidth - w;
    else left = (backingWidth - w) / 2;
        
    if (gContentMode == ARViewContentModeBottomLeft
        || gContentMode == ARViewContentModeBottom
        || gContentMode == ARViewContentModeBottomRight) bottom = 0;
    else if (gContentMode == ARViewContentModeTopLeft
             || gContentMode == ARViewContentModeTop
             || gContentMode == ARViewContentModeTopRight) bottom = backingHeight - h;
    else bottom = (backingHeight - h) / 2;

    glViewport(left, bottom, w, h);
    
    viewPort[viewPortIndexLeft] = left;
    viewPort[viewPortIndexBottom] = bottom;
    viewPort[viewPortIndexWidth] = w;
    viewPort[viewPortIndexHeight] = h;
    
#ifdef DEBUG
    LOGE("Viewport={%d, %d, %d, %d}\n", left, bottom, w, h);
#endif
    // Call through to anyone else who needs to know about changes in the ARView layout here.
    // --->
    VirtualEnvironmentHandleARViewUpdatedViewport(viewPort);

    gARViewLayoutRequired = false;
    
    return (true);
}


// All tasks which require both video and OpenGL to be inited should be performed here.
// References globals: cameraLens, gCparamLT, gPixFormat
// Modifies globals: gArglSettings
static bool initARView(void)
{
#ifdef DEBUG
    LOGI("Initialising ARView\n");
#endif        
    if (gARViewInited) return (false);
    
#ifdef DEBUG
    LOGI("Setting up argl.\n");
#endif        
    if ((gArglSettings = arglSetupForCurrentContext(&gCparamLT->param, gPixFormat)) == NULL) {
        LOGE("Unable to setup argl.\n");
        return (false);
    }
#ifdef DEBUG
    LOGI("argl setup OK.\n");
#endif        

    // Load objects (i.e. OSG models).
    VirtualEnvironmentInit(objectDataFilename);
    VirtualEnvironmentHandleARViewUpdatedCameraLens(cameraLens);
    
    gARViewInited = true;
#ifdef DEBUG
    LOGI("ARView initialised.\n");
#endif

    return (true);
}

JNIEXPORT void JNICALL JNIFUNCTION_NATIVE(nativeDrawFrame(JNIEnv* env, jobject obj))
{
	float width, height;
    
    if (!videoInited) {
#ifdef DEBUG
        LOGI("nativeDrawFrame !VIDEO\n");
#endif        
        return; // No point in trying to draw until video is inited.
    }
#ifdef DEBUG
    LOGI("nativeDrawFrame\n");
#endif        
    if (!gARViewInited) {
        if (!initARView()) return;
    }
    if (gARViewLayoutRequired) layoutARView();
    
    // Upload new video frame if required.
    if (videoFrameNeedsPixelBufferDataUpload) {
        arglPixelBufferDataUploadBiPlanar(gArglSettings, gVideoFrame, gVideoFrame + videoWidth*videoHeight);
        videoFrameNeedsPixelBufferDataUpload = false;
    }
    
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the buffers for new frame.
    
    // Display the current frame
    arglDispImage(gArglSettings);
    
    // Set up 3D mode.
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(cameraLens);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glStateCacheEnableDepthTest();

    // Set any initial per-frame GL state you require here.
    // --->
    
    // Lighting and geometry that moves with the camera should be added here.
    // (I.e. should be specified before camera pose transform.)
    // --->
    
    VirtualEnvironmentHandleARViewDrawPreCamera();
    
    if (cameraPoseValid) {
        
        glMultMatrixf(cameraPose);
        
        // All lighting and geometry to be drawn in world coordinates goes here.
        // --->
        VirtualEnvironmentHandleARViewDrawPostCamera();
    }
        
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();
    
    // Set up 2D mode.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	width = (float)viewPort[viewPortIndexWidth];
	height = (float)viewPort[viewPortIndexHeight];
	glOrthof(0.0f, width, 0.0f, height, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glStateCacheDisableDepthTest();

    // Add your own 2D overlays here.
    // --->
    
    VirtualEnvironmentHandleARViewDrawOverlay();
    
    // If you added external OpenGL code above, and that code doesn't use the glStateCache routines,
    // then uncomment the line below.
    //glStateCacheFlush();

#ifdef DEBUG
    // Example of 2D drawing. It just draws a white border line. Change the 0 to 1 to enable.
    const GLfloat square_vertices [4][2] = { {0.5f, 0.5f}, {0.5f, height - 0.5f}, {width - 0.5f, height - 0.5f}, {width - 0.5f, 0.5f} };
    glStateCacheDisableLighting();
    glStateCacheDisableTex2D();
    glVertexPointer(2, GL_FLOAT, 0, square_vertices);
    glStateCacheEnableClientStateVertexArray();
    glColor4ub(255, 255, 255, 255);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
#endif
}
