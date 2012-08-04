/*
 * MediaMixer.java
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

package org.murillo.mcuWeb;
import java.io.Serializable;
import java.net.MalformedURLException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.apache.xmlrpc.XmlRpcException;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import org.murillo.MediaServer.Codecs;
import org.murillo.MediaServer.XmlRpcBroadcasterClient;
import org.murillo.MediaServer.XmlRpcEventManager;
import org.murillo.MediaServer.XmlRpcMcuClient;
import org.murillo.util.SubNetInfo;
import org.murillo.util.ThreadPool;
/**
 *
 * @author Sergio Garcia Murillo
 */
public class MediaMixer implements Serializable {
    private String name;
    private String url;
    private String ip;
    private String publicIp;
    private SubNetInfo localNet;
    private Integer queueId;

    private HashSet<XmlRpcMcuClient> mcuClients;
    private XmlRpcMcuClient client;
    private XmlRpcEventManager eventManager;
    private String state;


    /** Creates a new instance of MediaMixer */
    public MediaMixer(String name,String url,String ip,String publicIp,String localNet) throws MalformedURLException {
            //Save Values
            this.name = name;
            this.url = url;
            this.ip = ip;
            this.publicIp = publicIp;
            this.queueId = 0;
            //Create default client
            client = new XmlRpcMcuClient(url + "/mcu");
            eventManager = new XmlRpcEventManager(this.getUID());
            //Create client list
            mcuClients = new HashSet<XmlRpcMcuClient>();
            try {
                //parse it
                this.localNet = new SubNetInfo(localNet);
            } catch (UnknownHostException ex) {
                //Log
                Logger.getLogger(MediaMixer.class.getName()).log(Level.SEVERE, null, ex);
                //Create empty one
                this.localNet = new SubNetInfo(new byte[]{0,0,0,0},0);
            }
            //NO state
            state = "";
    }

    public String getName() {
        return name;
    }

    public String getUrl() {
        return url;
    }
    
    public String getIp() {
        return ip;
    }

    public String getPublicIp() {
        return publicIp;
    }

    public SubNetInfo getLocalNet() {
        return localNet;
    }

    public boolean isNated(String ip){
        try {
            //Check if it is a private network address  and not in local address
            if (SubNetInfo.isPrivate(ip) && !localNet.contains(ip))
                //It is nated
                return true;
        } catch (UnknownHostException ex) {
            //Log
            Logger.getLogger(MediaMixer.class.getName()).log(Level.WARNING, "Wrong IP address, doing NAT {0}", ip);
            //Do nat
            return true;
        }

        //Not nat
        return false;
    }

    public String getUID() {
        return name+"@"+url;
    }
    
    public XmlRpcBroadcasterClient createBroadcastClient() {
        XmlRpcBroadcasterClient client = null;
        try {
            client = new XmlRpcBroadcasterClient(url + "/broadcaster");
        } catch (MalformedURLException ex) {
            Logger.getLogger(MediaMixer.class.getName()).log(Level.SEVERE, null, ex);
        }
        return client;
    }
    
    public void connectEveneManager(final XmlRpcEventManager.Listener listener) {
        ThreadPool.Execute(new Runnable() {
            @Override
            public void run() {
                try {
                    queueId = client.EventQueueCreate();
                    eventManager.Connect(url + "/events/mcu/" + getQueueId(), listener);
                } catch (XmlRpcException ex) {
                    Logger.getLogger(MediaMixer.class.getName()).log(Level.SEVERE, "event queue create error");
                    try {
                        eventManager.Connect(url + "/events/mcu/" + getQueueId(), listener);
                    } catch (MalformedURLException ex1) {
                        Logger.getLogger(MediaMixer.class.getName()).log(Level.SEVERE, "eventManager connect error");
                    }
                } catch (MalformedURLException ex) {
                    Logger.getLogger(MediaMixer.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        });
        Logger.getLogger(MediaMixer.class.getName()).log(Level.INFO, "connectEveneManager");
    }

    public void disconnectEveneManager() {
        try {
            client.EventQueueDelete(queueId);
            queueId = 0;
        } catch (XmlRpcException ex) {
            Logger.getLogger(MediaMixer.class.getName()).log(Level.SEVERE, "event queue delete error");
        }
        eventManager.Cancel();
        Logger.getLogger(MediaMixer.class.getName()).log(Level.INFO, "disconnectEventManager");
    }
    
    public XmlRpcMcuClient createMcuClient() {
        XmlRpcMcuClient mcuClient = null;
        try {
            //Create client
            mcuClient = new XmlRpcMcuClient(url + "/mcu");
            //Append to set
            mcuClients.add(mcuClient);
        } catch (MalformedURLException ex) {
            Logger.getLogger(MediaMixer.class.getName()).log(Level.SEVERE, null, ex);
        }
        return mcuClient;
    }
    
    public static HashMap<Integer,String> getSizes() {
        //The map
        HashMap<Integer,String> sizes = new HashMap<Integer,String>();
        //Add values
        sizes.put(XmlRpcMcuClient.QCIF,"QCIF");
        sizes.put(XmlRpcMcuClient.CIF,"CIF");
        sizes.put(XmlRpcMcuClient.VGA,"VGA");
        sizes.put(XmlRpcMcuClient.PAL,"PAL");
        sizes.put(XmlRpcMcuClient.HVGA,"HVGA");
        sizes.put(XmlRpcMcuClient.QVGA,"QVGA");
        sizes.put(XmlRpcMcuClient.HD720P,"HD720P");
        //Return map
        return sizes;
    }
    
    public static HashMap<Integer,String> getAudioCodecs() {
        //The map
        HashMap<Integer,String> codecs = new HashMap<Integer,String>();
        //Add values
        codecs.put(Codecs.PCMU,"PCMU");
        codecs.put(Codecs.PCMA,"PCMA");
        codecs.put(Codecs.GSM,"GSM");
        //Return map
        return codecs;
    } 
    
    public static HashMap<Integer,String> getVideoCodecs() {
        //The map
        HashMap<Integer,String> codecs = new HashMap<Integer,String>();
        //Add values
        codecs.put(Codecs.H263_1996,"H263-1996");
        codecs.put(Codecs.H263_1998,"H263-1998");
        codecs.put(Codecs.H263_1998,"H263-2000");
        codecs.put(Codecs.MPEG4,"MPEG4");
        codecs.put(Codecs.MPEG4,"H264");
        //Return map
        return codecs;
    } 
    
    public static HashMap<Integer,String> getTextCodecs() {
        //The map
        HashMap<Integer,String> codecs = new HashMap<Integer,String>();
        //Add values
        codecs.put(Codecs.T140,   "T140");
        codecs.put(Codecs.T140RED,"T140RED");
        //Return map
        return codecs;
    }

    public static HashMap<Integer,String> getMosaics() {
        //The map
        HashMap<Integer,String> mosaics = new HashMap<Integer,String>();
        //Add values
        mosaics.put(XmlRpcMcuClient.MOSAIC1x1 ,"MOSAIC1x1");
        mosaics.put(XmlRpcMcuClient.MOSAIC2x2 ,"MOSAIC2x2");
        mosaics.put(XmlRpcMcuClient.MOSAIC3x3 ,"MOSAIC3x3");
        mosaics.put(XmlRpcMcuClient.MOSAIC3p4 ,"MOSAIC3+4");
        mosaics.put(XmlRpcMcuClient.MOSAIC1p7 ,"MOSAIC1+7");
        mosaics.put(XmlRpcMcuClient.MOSAIC1p5 ,"MOSAIC1+5");
        mosaics.put(XmlRpcMcuClient.MOSAIC1p1 ,"MOSAIC1+1");
        mosaics.put(XmlRpcMcuClient.MOSAICPIP1,"MOSAICPIP1");
        mosaics.put(XmlRpcMcuClient.MOSAICPIP3,"MOSAICPIP3");
        //Return map
        return mosaics;
    }

    public String getState() {
        return state;
    }

    public void releaseMcuClient(XmlRpcMcuClient client) {
        //Release client
        mcuClients.remove(client);
    }

    
    public JSONObject toJSONObject() throws JSONException {
        JSONObject json = new JSONObject();
        json.put("name", getName());
        json.put("url", getUrl());
        return json;
    }

    /**
     * @return the queueId
     */
    public Integer getQueueId() {
        return queueId;
    }
}
