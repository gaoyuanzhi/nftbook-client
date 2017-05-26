/*
 *  CameraSurface.java
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
 *  Copyright 2015 Daqri, LLC.
 *  Copyright 2011-2015 ARToolworks, Inc.
 *
 *  Author(s): Julian Looser, Philip Lamb
 *
 */

package org.artoolkit.ar.samples.nftBook;

import java.io.IOException;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.SocketException;
import java.util.Arrays;
import java.lang.String;
import java.lang.Throwable;
import java.nio.*;

import org.artoolkit.ar.samples.nftBook.R;
import android.annotation.SuppressLint;
import android.content.Context;
import android.graphics.PixelFormat;
import android.hardware.Camera;
import android.os.Build;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

public class CameraSurface extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {

	private static final String TAG = "CameraSurface";
	private Camera camera;
	DatagramSocket SendSocket;
	UdpSendThread udpSendThread;
	Boolean socket_setup = false;
	private static short frame_id_update = 0;
    private static byte[] frame;

    @SuppressWarnings("deprecation")
	public CameraSurface(Context context) {
        
    	super(context);
        
    	SurfaceHolder holder = getHolder();     
        holder.addCallback(this);
        holder.setType(SurfaceHolder.SURFACE_TYPE_PUSH_BUFFERS); // Deprecated in API level 11. Still required for API levels <= 10.
        
    }
 
    // SurfaceHolder.Callback methods

    @SuppressLint("NewApi")
    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        
		Log.i(TAG, "Opening camera.");
    	try {
    		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
    			int cameraIndex = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(getContext()).getString("pref_cameraIndex", "0"));
    			camera = Camera.open(cameraIndex);
    		} else {
    			camera = Camera.open();
    		}
    	} catch (RuntimeException exception) {
    		Log.e(TAG, "Cannot open camera. It may be in use by another process.");
    	}
    	if (camera != null) {
    		try {
        	
    			camera.setPreviewDisplay(holder);
    			//camera.setPreviewCallback(this);
    			camera.setPreviewCallbackWithBuffer(this); // API level 8 (Android 2.2)
       	
    		} catch (IOException exception) {
        		Log.e(TAG, "Cannot set camera preview display.");
        		camera.release();
        		camera = null;  
    		}
    	}
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    	if (camera != null) {  	
    		Log.i(TAG, "Closing camera.");
    		camera.stopPreview();
    		camera.setPreviewCallback(null);
    		camera.release();
    		camera = null;
    	}
    }

 
    @SuppressLint("NewApi") // CameraInfo
	@SuppressWarnings("deprecation") // setPreviewFrameRate
	@Override
    public void surfaceChanged(SurfaceHolder holder, int format, int w, int h) {
    	
    	if (camera != null) {

    		String camResolution = PreferenceManager.getDefaultSharedPreferences(getContext()).getString("pref_cameraResolution", getResources().getString(R.string.pref_defaultValue_cameraResolution));
            String[] dims = camResolution.split("x", 2);
            Camera.Parameters parameters = camera.getParameters();
            parameters.setPreviewSize(Integer.parseInt(dims[0]), Integer.parseInt(dims[1]));
            parameters.setPreviewFrameRate(30);
            camera.setParameters(parameters);        
            
            parameters = camera.getParameters();
    		int capWidth = parameters.getPreviewSize().width;
    		int capHeight = parameters.getPreviewSize().height;
            int pixelformat = parameters.getPreviewFormat(); // android.graphics.imageformat
            PixelFormat pixelinfo = new PixelFormat();
            PixelFormat.getPixelFormatInfo(pixelformat, pixelinfo);
            int cameraIndex = 0;
            boolean frontFacing = false;
    		if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.GINGERBREAD) {
    			Camera.CameraInfo cameraInfo = new Camera.CameraInfo();
    			cameraIndex = Integer.parseInt(PreferenceManager.getDefaultSharedPreferences(getContext()).getString("pref_cameraIndex", "0"));
    			Camera.getCameraInfo(cameraIndex, cameraInfo);
    			if (cameraInfo.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) frontFacing = true;
    		}

    		int bufSize = capWidth * capHeight * pixelinfo.bitsPerPixel / 8; // For the default NV21 format, bitsPerPixel = 12.
            
            for (int i = 0; i < 5; i++) camera.addCallbackBuffer(new byte[bufSize]);
            
            camera.startPreview();

            nftBookActivity.nativeVideoInit(capWidth, capHeight, cameraIndex, frontFacing);

    	}
    }

    // Camera.PreviewCallback methods.
    
	@Override
	public void onPreviewFrame(byte[] data, Camera cam) {
		
		nftBookActivity.nativeVideoFrame(data);
        if (frame_id_update < 32767)
		    frame_id_update ++;
        else
            frame_id_update = 0;
        frame = data;
		if (!socket_setup) {
			socket_setup = true;
			try {
				// get port and address
				//int server_port = 48010;
                int server_port = 9999;
				//int client_port = 8888;
                int client_port = 10000;
				//InetAddress server_addr = InetAddress.getByName("131.179.80.180");
                //InetAddress server_addr = InetAddress.getByName("192.168.0.103");
                InetAddress server_addr = InetAddress.getByName("131.179.176.34");

				SendSocket = new DatagramSocket(client_port);
				udpSendThread = new UdpSendThread(SendSocket, server_port, server_addr);
				udpSendThread.start();
			} catch (Exception e) {
				Log.e(TAG, "exception", e);
				return;
			}
		}


		cam.addCallbackBuffer(data);
	}

	private class UdpSendThread extends Thread{

		DatagramSocket SendSocket;
		int ServerPort;
		InetAddress ServerAddr;
		boolean running = true;
		byte[] message;
        short frame_id = 0;

		public UdpSendThread(DatagramSocket _sendsocket, int _serverport, InetAddress _serveraddr) throws SocketException {
			super();
			this.SendSocket = _sendsocket;
			this.ServerPort = _serverport;
			this.ServerAddr = _serveraddr;
		}

		@Override
		protected void finalize() throws Throwable {
			super.finalize();
		}

		public void setRunning(boolean _running){
			this.running = _running;
			closeSockets();
		}

		private void closeSockets() {
			if (SendSocket != null) {
				SendSocket.close();
			}
		}

		@Override
		public void run() {
			try {
                int total_packet_sent = 0;
				// send the message
				while (running) {
                    if (frame_id_update == 0)
                        frame_id = 0;

					if (frame_id <= frame_id_update) {
						int sent_buffer_size = 0;
                        byte segment_id = 0;
						while (sent_buffer_size < frame.length) {
							int length_to_send = 1100;
                            byte last_segment_tag = 0;
							if (sent_buffer_size + length_to_send > frame.length)
							{
								length_to_send = frame.length - sent_buffer_size;
                                last_segment_tag = 1;
							}

							byte[] remaining_message = Arrays.copyOfRange(frame, sent_buffer_size, sent_buffer_size + length_to_send);
                            byte[] full_message = new byte[4+remaining_message.length];
                            full_message[0] = (byte)(frame_id & 0xff);
                            full_message[1] = (byte)((frame_id >> 8) & 0xff);
                            full_message[2] =  segment_id;
                            full_message[3] =  last_segment_tag;

                            byte[] metadata = {full_message[0], full_message[1]};
                            ByteBuffer wrapped = ByteBuffer.wrap(metadata);
                            short md = wrapped.getShort();
                            System.arraycopy(remaining_message, 0, full_message, 4, remaining_message.length);
							DatagramPacket p = new DatagramPacket(full_message, full_message.length, ServerAddr, ServerPort);
                            total_packet_sent ++;
							Log.d(TAG, "Message sent: " + Integer.toString(md));
							SendSocket.send(p);
							sent_buffer_size += length_to_send;
                            segment_id ++;
						}
						frame_id++;
					}
				}

				// receiving the message
//				while(running) {
//					byte[] buf = new byte[MAXPKTSIZE];
//					DatagramPacket packet = new DatagramPacket(buf, buf.length);
//					SendSocket.receive(packet);
//					String receive_string = new String(packet.getData(), 0, packet.getLength() );
//					Log.d(TAG, "message receive: " + receive_string);
//				}
			} catch (Exception e) {
				Log.e(TAG, "udp thread exception", e);
			} finally {
				closeSockets();
			}
		}
	}
 
}
