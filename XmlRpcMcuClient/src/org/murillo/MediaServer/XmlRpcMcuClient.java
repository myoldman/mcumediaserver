/*
 * XmlRpcMcuClient.java
 *
 * Copyright (C) 2007  Sergio Garcia Murillo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
package org.murillo.MediaServer;

import java.net.MalformedURLException;
import java.net.URL;
import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.apache.xmlrpc.client.XmlRpcCommonsTransportFactory;

/**
 *
 * @author Sergio Garcia Murillo
 */
public class XmlRpcMcuClient {

    public class MediaStatistics {
	public boolean	isSending;
	public boolean	isReceiving;
        public Integer	lostRecvPackets;
	public Integer	numRecvPackets;
	public Integer	numSendPackets;
	public Integer	totalRecvBytes;
	public Integer	totalSendBytes;
    };

    public class ConferenceInfo {
        public Integer id;
        public String name;
        public Integer numPart;
    }
    public static final Integer QCIF = 0;
    public static final Integer CIF  = 1;
    public static final Integer VGA  = 2;
    public static final Integer PAL  = 3;
    public static final Integer HVGA  = 4;
    public static final Integer QVGA = 5;
    public static final Integer HD720P = 6;
    
    public static final Integer MOSAIC1x1   = 0;
    public static final Integer MOSAIC2x2   = 1;
    public static final Integer MOSAIC3x3   = 2;
    public static final Integer MOSAIC3p4   = 3;
    public static final Integer MOSAIC1p7   = 4;
    public static final Integer MOSAIC1p5   = 5;
    public static final Integer MOSAIC1p1   = 6;
    public static final Integer MOSAICPIP1  = 7;
    public static final Integer MOSAICPIP3  = 8;

    public static final Integer DefaultMosaic = 0;

    public static final Integer RTP = 0;
    public static final Integer RTMP = 1;
    
    public static final int getMosaicNumSlots(Integer type) 
    {
        switch(type) 
        {
            case 0:
                return 1;
            case 1:
                return 4;
            case 2:
                return 9;
            case 3:
                return 7;
            case 4:
                return 8;
            case 5:
                return 6;
            case 6:
                return 2;
            case 7:
                return 2;
            case 8:
                return 4;
        }
        
        return -1;
    }

    
    private XmlRpcClient client;
    private XmlRpcClientConfigImpl config;
    
    /** Creates a new instance of XmlRpcMcuClient */
    public XmlRpcMcuClient(String  url) throws MalformedURLException
    {
        config = new XmlRpcClientConfigImpl();
        config.setServerURL(new URL(url));
        client = new XmlRpcClient();
        //XmlRpcCommonsTransportFactory  factory = new XmlRpcCommonsTransportFactory (client);
        //client.setTransportFactory(factory);
        client.setConfig(config);
    }

    public Map<Integer,ConferenceInfo> getConferences() throws XmlRpcException {
        //Create request
        Object[] request = new Object[]{};
        //Execute
        HashMap response = (HashMap) client.execute("GetConferences", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Create map
        HashMap<Integer,ConferenceInfo> conferences = new HashMap<Integer, ConferenceInfo>(returnVal.length);
        //For each value in array
        for (int i=0;i<returnVal.length;i++)
        {
            //Get array
             Object[] arr = (Object[]) returnVal[i];
             //Get id
             Integer id = (Integer)arr[0];
             //Create info
             ConferenceInfo info = new ConferenceInfo();
             //Fill values
             info.id      = (Integer)arr[0];
             info.name    = (String)arr[1];
             info.numPart = (Integer)arr[2];
             //Add it
             conferences.put(id, info);
        }
        //Return conference list
        return conferences;
    }

    public Integer CreateConference(String tag) throws XmlRpcException
    {
        //Create conference without event queue
        return CreateConference(tag,0);
    }

    public Integer CreateConference(String tag,Integer queueId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{tag,queueId};
        //Execute 
        HashMap response = (HashMap) client.execute("CreateConference", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return conference id
        return (Integer)returnVal[0];
    }

    public Integer CreateMosaic(Integer confId,Integer comp,Integer size) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,comp,size};
        //Execute
        HashMap response = (HashMap) client.execute("CreateMosaic", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return conference id
        return (Integer)returnVal[0];
    }
    public Boolean SetMosaicOverlayImage(Integer confId,Integer mosaicId,String filename) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,mosaicId,filename};
        //Execute
        HashMap response = (HashMap) client.execute("SetMosaicOverlayImage", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return port
        return ((Integer)returnVal[0])==1;
    }
    
    public Boolean ResetMosaicOverlay(Integer confId,Integer mosaicId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,mosaicId};
        //Execute
        HashMap response = (HashMap) client.execute("ResetMosaicOverlay", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return port
        return ((Integer)returnVal[0])==1;
    }

    public Boolean DeleteMosaic(Integer confId,Integer mosaicId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,mosaicId};
        //Execute
        HashMap response = (HashMap) client.execute("DeleteMosaic", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return port
        return ((Integer)returnVal[0])==1;
    }
    
    public Integer CreateSpy(Integer confId, Integer spyeeId) throws XmlRpcException
    {
       //Create request
        Object[] request = new Object[]{confId,spyeeId};
        //Execute 
        HashMap response = (HashMap) client.execute("CreateSpy", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return part id
        return (Integer)returnVal[0];  
    }
    
    public Integer CreateParticipant(Integer confId,Integer mosaicId,String name,Integer type) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,mosaicId,name,type};
        //Execute 
        HashMap response = (HashMap) client.execute("CreateParticipant", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return part id
        return (Integer)returnVal[0];
    }
    
    public boolean SetCompositionType(Integer confId,Integer mosaicId,Integer comp,Integer size) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,mosaicId,comp,size};
        //Execute 
        HashMap response = (HashMap) client.execute("SetCompositionType", request);
        //Return 
        return true;
    }
    
    public boolean SetMosaicSlot(Integer confId,Integer mosaicId,Integer num,Integer id) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,mosaicId,num,id};
        //Execute 
        HashMap response = (HashMap) client.execute("SetMosaicSlot", request);
        //Return 
        return true;
    }

    public boolean AddMosaicParticipant(Integer confId,Integer mosaicId,Integer partId) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,mosaicId,partId};
        //Execute
        HashMap response = (HashMap) client.execute("AddMosaicParticipant", request);
        //Return
        return true;
    }

    public boolean RemoveMosaicParticipant(Integer confId,Integer mosaicId,Integer partId) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,mosaicId,partId};
        //Execute
        HashMap response = (HashMap) client.execute("RemoveMosaicParticipant", request);
        //Return
        return true;
    }
    
    //Video
    public boolean SetVideoCodec(Integer confId,Integer partId,Integer codec,Integer mode,Integer fps,Integer bitrate,Integer quality, Integer fillLevel, Integer intraPeriod) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,codec,mode,fps,bitrate,quality,fillLevel,intraPeriod};
        //Execute 
        HashMap response = (HashMap) client.execute("SetVideoCodec", request);
        //Return 
        return true;
    }
        
    public boolean StartSendingVideoSpy(Integer confId,Integer spyId,String sendVideoIp,Integer sendVideoPort,HashMap<Integer,Integer> rtpMap) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,spyId,sendVideoIp,sendVideoPort,rtpMap};
        //Execute 
        HashMap response = (HashMap) client.execute("StartSendingVideoSpy", request);
        //Return 
        return true;
    }
             
    public boolean StartSendingVideo(Integer confId,Integer partId,String sendVideoIp,Integer sendVideoPort,HashMap<Integer,Integer> rtpMap) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,sendVideoIp,sendVideoPort,rtpMap};
        //Execute 
        HashMap response = (HashMap) client.execute("StartSendingVideo", request);
        //Return 
        return true;
    }
    
    public boolean StopSendingVideo(Integer confId,Integer partId) throws XmlRpcException
    {
       //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute 
        HashMap response = (HashMap) client.execute("StopSendingVideo", request);
        //Return 
        return true;
    }
    
    public Integer StartReceivingVideoSpy(Integer confId, Integer spyId, HashMap<Integer,Integer> rtpMap) throws XmlRpcException{
         //Create request
        Object[] request = new Object[]{confId,spyId,rtpMap};
        //Execute 
        HashMap response = (HashMap) client.execute("StartReceivingVideoSpy", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return port
        return (Integer)returnVal[0];
    }
    
    public Integer StartReceivingVideo(Integer confId,Integer partId,HashMap<Integer,Integer> rtpMap) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,rtpMap};
        //Execute 
        HashMap response = (HashMap) client.execute("StartReceivingVideo", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return port
        return (Integer)returnVal[0];
    }
    
    public boolean StopReceivingVideo(Integer confId,Integer partId) throws XmlRpcException
    {
       //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute 
        HashMap response = (HashMap) client.execute("StopReceivingVideo", request);
        //Return 
        return true;
    }
    
    //Audio
    public boolean SetAudioCodec(Integer confId,Integer partId,Integer codec) throws XmlRpcException
    {
       //Create request
        Object[] request = new Object[]{confId,partId,codec};
        //Execute 
        HashMap response = (HashMap) client.execute("SetAudioCodec", request);
        //Return 
        return true;
    }
    
    public boolean StartSendingAudio(Integer confId,Integer partId,String sendAudioIp,Integer sendAudioPort,HashMap<Integer,Integer> rtpMap) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,sendAudioIp,sendAudioPort,rtpMap};
        //Execute 
        HashMap response = (HashMap) client.execute("StartSendingAudio", request);
        //Return 
        return true;
    }
    
    public boolean StopSendingAudio(Integer confId,Integer partId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute 
        HashMap response = (HashMap) client.execute("StopSendingAudio", request);
        //Return 
        return true;
    }
    
    public Integer StartReceivingAudio(Integer confId,Integer partId,HashMap<Integer,Integer> rtpMap) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,rtpMap};
        //Execute 
        HashMap response = (HashMap) client.execute("StartReceivingAudio", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return port
        return (Integer)returnVal[0];
    }
    
    public boolean StopReceivingAudio(Integer confId,Integer partId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute 
        HashMap response = (HashMap) client.execute("StopReceivingAudio", request);
        //Return 
        return true;
    }
    
    //Text
    public boolean SetTextCodec(Integer confId,Integer partId,Integer codec) throws XmlRpcException
    {
       //Create request
        Object[] request = new Object[]{confId,partId,codec};
        //Execute
        HashMap response = (HashMap) client.execute("SetTextCodec", request);
        //Return
        return true;
    }

    public boolean StartSendingText(Integer confId,Integer partId,String sendTextIp,Integer sendTextPort,HashMap<Integer,Integer> rtpMap) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,sendTextIp,sendTextPort,rtpMap};
        //Execute
        HashMap response = (HashMap) client.execute("StartSendingText", request);
        //Return
        return true;
    }

    public boolean StopSendingText(Integer confId,Integer partId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute
        HashMap response = (HashMap) client.execute("StopSendingText", request);
        //Return
        return true;
    }

    public Integer StartReceivingText(Integer confId,Integer partId,HashMap<Integer,Integer> rtpMap) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,rtpMap};
        //Execute
        HashMap response = (HashMap) client.execute("StartReceivingText", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return port
        return (Integer)returnVal[0];
    }

    public boolean StopReceivingText(Integer confId,Integer partId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute
        HashMap response = (HashMap) client.execute("StopReceivingText", request);
        //Return
        return true;
    }

    public boolean DeleteParticipant(Integer confId,Integer partId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute 
        HashMap response = (HashMap) client.execute("DeleteParticipant", request);
        //Return 
        return true;
    }

    public boolean DeleteSpy(Integer confId,Integer spyId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,spyId};
        //Execute 
        HashMap response = (HashMap) client.execute("DeleteSpy", request);
        //Return 
        return true;
    }
        
    public boolean StartBroadcaster(Integer confId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId};
        //Execute
        HashMap response = (HashMap) client.execute("StartBroadcaster", request);
        //Return
        return true;
    }

    public boolean StopBroadcaster(Integer confId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId};
        //Execute 
        HashMap response = (HashMap) client.execute("StopBroadcaster", request);
        //Return 
        return true;
    }
    
    public boolean DeleteConference(Integer confId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId};
        //Execute 
        HashMap response = (HashMap) client.execute("DeleteConference", request);
        //Return 
        return true;
    }

    public void AddConferencetToken(Integer confId,String token) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,token};
        //Execute
        HashMap response = (HashMap) client.execute("AddConferenceToken", request);
    }

    public void AddParticipantInputToken(Integer confId,Integer partId,String token) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,partId,token};
        //Execute
        HashMap response = (HashMap) client.execute("AddParticipantInputToken", request);
    }

    public void AddParticipantOutputToken(Integer confId,Integer partId,String token) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,partId,token};
        //Execute
        HashMap response = (HashMap) client.execute("AddParticipantOutputToken", request);
    }

    public int CreatePlayer(Integer confId,Integer privateId,String name) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{confId,privateId,name};
        //Execute
        HashMap response = (HashMap) client.execute("CreatePlayer", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return part id
        return (Integer)returnVal[0];
    }

    public boolean DeletePlayer(Integer confId,int playerId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,playerId};
        //Execute
        HashMap response = (HashMap) client.execute("DeletePlayer", request);
        //Return
        return true;
    }
    public boolean StartPlaying(Integer confId,int playerId,String filename,int loop) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,playerId,filename,loop};
        //Execute
        HashMap response = (HashMap) client.execute("StartPlaying", request);
        //Return
        return true;
    }
    public boolean StopPlaying(Integer confId,int playerId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,playerId};
        //Execute
        HashMap response = (HashMap) client.execute("StopPlaying", request);
        //Return
        return true;
    }
    public boolean StartRecordingParticipant(Integer confId,int playerId,String filename) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,playerId,filename};
        //Execute
        HashMap response = (HashMap) client.execute("StartRecordingParticipant", request);
        //Return
        return true;
    }

    public boolean StopRecordingParticipant(Integer confId,int playerId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,playerId};
        //Execute
        HashMap response = (HashMap) client.execute("StopRecordingParticipant", request);
        //Return
        return true;
    }

    public void SetParticipantMosaic(Integer confId,Integer partId, Integer mosaicId) throws XmlRpcException {
        //Create request
        Object[] request = new Object[]{confId,partId,mosaicId};
        //Execute
        HashMap response = (HashMap) client.execute("SetParticipantMosaic", request);
    }

    public boolean SetMute(Integer confId,int partId,Codecs.MediaType media,boolean isMuted) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId,media.valueOf(),isMuted?1:0};
        //Execute
        HashMap response = (HashMap) client.execute("SetMute", request);
        //Return
        return true;
    }

    public boolean SendFPU(Integer confId,int partId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute
        HashMap response = (HashMap) client.execute("SendFPU", request);
        //Return
        return true;
    }

    public Map<String,MediaStatistics> getParticipantStatistics(Integer confId, Integer partId) throws XmlRpcException {
        //Create request
        Object[] request = new Object[]{confId,partId};
        //Execute
        HashMap response = (HashMap) client.execute("GetParticipantStatistics", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Create map
        HashMap<String,MediaStatistics> partStats = new HashMap<String, MediaStatistics>();
        //For each value in array
        for (int i=0;i<returnVal.length;i++)
        {
            //Get array
             Object[] arr = (Object[]) returnVal[i];
             //Get media
             String media = (String)arr[0];
             //Create stats
             MediaStatistics stats = new MediaStatistics();
             //Fill values
             stats.isReceiving      = ((Integer)arr[1])==1;
             stats.isSending        = ((Integer)arr[2])==1;
             stats.lostRecvPackets  = (Integer)arr[3];
             stats.numRecvPackets   = (Integer)arr[4];
             stats.numSendPackets   = (Integer)arr[5];
             stats.totalRecvBytes   = (Integer)arr[6];
             stats.totalSendBytes   = (Integer)arr[7];
             //Add it
             partStats.put(media, stats);
        }
        //Return conference id
        return partStats;
    }
    
    public int EventQueueCreate() throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{};
        //Execute
        HashMap response = (HashMap) client.execute("EventQueueCreate", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return part id
        return (Integer)returnVal[0];
    }

    public boolean EventQueueDelete(int queueId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{queueId};
        //Execute
        HashMap response = (HashMap) client.execute("EventQueueDelete", request);
        //Return
        return (((Integer)response.get("returnCode"))==1);
    }
 }
