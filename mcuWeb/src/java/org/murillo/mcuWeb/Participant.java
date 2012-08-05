/*
 * Participant.java
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

import com.ffcs.mcu.SipEndPointManager;
import com.ffcs.mcu.Spyer;
import com.ffcs.mcu.pojo.SipEndPoint;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.io.Serializable;
import java.util.HashSet;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.mail.BodyPart;
import javax.mail.MessagingException;
import javax.mail.Multipart;
import javax.mail.internet.MimeBodyPart;
import javax.mail.internet.MimeMultipart;
import javax.servlet.sip.Address;
import javax.servlet.sip.SipApplicationSession;
import javax.servlet.sip.SipFactory;
import javax.servlet.sip.SipServletRequest;
import javax.servlet.sip.SipServletResponse;
import javax.servlet.sip.SipSession;
import javax.servlet.sip.SipURI;
import org.apache.xmlrpc.XmlRpcException;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import org.murillo.MediaServer.Codecs.MediaType;
import org.murillo.MediaServer.XmlRpcMcuClient;

/**
 *
 * @author Sergio Garcia Murillo
 */
public abstract class Participant implements Serializable {

    /**
     * @return the admin
     */
    public boolean isAdmin() {
        return admin;
    }

    /**
     * @param admin the admin to set
     */
    public void setAdmin(boolean admin) {
        this.admin = admin;
    }

    /**
     * @return the displayName
     */
    public String getDisplayName() {
        return displayName;
    }

    /**
     * @param displayName the displayName to set
     */
    public void setDisplayName(String displayName) {
        this.displayName = displayName;
    }
    protected Integer id;
    protected Type type;
    protected String name;
    protected Profile profile;
    protected Boolean audioMuted;
    protected Boolean videoMuted;
    protected Boolean textMuted;
    protected Boolean audioSupported;
    protected Boolean videoSupported;
    protected Boolean textSupported;
    protected State state;
    protected Integer mosaicId;
    protected String displayName;
    protected String sendIp;
    protected HashSet<Listener> listeners = null;
    protected Conference conf = null;
    private boolean autoAccept;
    protected boolean admin;
    protected boolean is_desktop_share;
    protected boolean isDestroying;
    protected XmlRpcMcuClient client;
    protected Map<Integer, Spyer> spyers;

    /**
     * @return the sendIp
     */
    public String getSendIp() {
        return sendIp;
    }

    /**
     * @param sendIp the sendIp to set
     */
    public void setSendIp(String sendIp) {
        this.sendIp = sendIp;
    }

    public interface Listener {

        public void onStateChanged(Participant part, State state);
    };

    public enum State {

        CREATED, CONNECTING, WAITING_ACCEPT, CONNECTED, ERROR, TIMEOUT, BUSY, DECLINED, NOTFOUND, DISCONNECTED, DESTROYED
    }

    public static enum Type {

        SIP("SIP", XmlRpcMcuClient.RTP),
        RTSP("RTSP", XmlRpcMcuClient.RTP),
        WEB("WEB", XmlRpcMcuClient.RTMP),
        SPY("SPY", XmlRpcMcuClient.RTP);
        public final String name;
        public final Integer value;

        Type(String name, Integer value) {
            this.name = name;
            this.value = value;
        }

        public Integer valueOf() {
            return value;
        }

        public String getName() {
            return name;
        }
    };

    Participant() {
        //Default constructor for Xml Serialization
    }

    Participant(Integer id, String name, Integer mosaicId, Conference conf, Type type) {
        //Save values
        this.id = id;
        this.conf = conf;
        this.type = type;
        this.name = name;
        this.mosaicId = mosaicId;
        //Get initial profile
        this.profile = conf.getProfile();
        //Not muted
        this.audioMuted = false;
        this.videoMuted = false;
        this.textMuted = false;
        //Supported media
        this.audioSupported = true;
        this.videoSupported = true;
        this.textSupported = true;
        //Autoaccept by default
        autoAccept = false;
        //Create listeners
        listeners = new HashSet<Listener>();
        spyers = new HashMap<Integer, Spyer>();
        //Initial state
        state = State.CREATED;
        admin = false;
        this.is_desktop_share = false;
        isDestroying = false;
        //Create the client
        this.client = conf.getMixer().createMcuClient();
    }
//

    public Conference getConference() {
        return conf;
    }

    public Integer getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    public boolean isAutoAccept() {
        return autoAccept;
    }

    public void setAutoAccept(boolean autoAccept) {
        this.autoAccept = autoAccept;
    }

    public Profile getVideoProfile() {
        return profile;
    }

    protected void error(State state, String message) {
        //Set the state
        setState(state);
        //Check cdr
        //Teminate
        destroy();
    }

    protected void error(State state, String message, Integer code) {
        //Set the state
        setState(state);
        //Teminate
        destroy();
    }

    protected void setState(State state) {
        if (this.getType() == Type.SIP && !this.is_desktop_share) {
            if (state == State.CONNECTING) {
                SipEndPointManager.INSTANCE.setSipEndPointStateByNumber(this.getUsername(), com.ffcs.mcu.pojo.EndPoint.State.CONNECTING);
            } else if (state == State.CONNECTED) {
                SipEndPointManager.INSTANCE.setSipEndPointStateByNumber(this.getUsername(), com.ffcs.mcu.pojo.EndPoint.State.CONNECTED);
            } else {
                if (state == State.DISCONNECTED) {
                    this.admin = false;
                    conf.changeAdmin(id);
                    for(Spyer spyer : spyers.values()){
                        spyer.end();
                    }
                    spyers.clear();
                    //SipEndPointManager.INSTANCE.setSipEndPointStateByNumber(this.getUsername(), com.ffcs.mcu.pojo.EndPoint.State.DISCONNECTED);
                }
                if (state != State.CREATED && state != State.DESTROYED) {
                    SipEndPointManager.INSTANCE.setSipEndPointStateByNumber(this.getUsername(), com.ffcs.mcu.pojo.EndPoint.State.IDLE);
                }
            }
        }
        
        //Change it
        this.state = state;
        
        //Call listeners
        for (Listener listener : listeners) //Call it
        {
            listener.onStateChanged(this, state);
        }

    }

    public void setName(String name) {
        this.name = name;
    }

    public State getState() {
        return state;
    }

    public Boolean getAudioSupported() {
        return audioSupported;
    }

    public Boolean getTextSupported() {
        return textSupported;
    }

    public Boolean getVideoSupported() {
        return videoSupported;
    }

    public void setListener(Listener listener) {
        listeners.add(listener);
    }

    public Type getType() {
        return type;
    }

    public void setAudioMuted(Boolean flag) {
        try {
            //Get client
            //XmlRpcMcuClient client = conf.getMCUClient();
            //Delete participant
            client.SetMute(conf.getId(), id, MediaType.AUDIO, flag);
            //Set audio muted
            audioMuted = flag;
        } catch (XmlRpcException ex) {
            Logger.getLogger(Participant.class.getName()).log(Level.SEVERE, "Failed to mute participant.", ex);
        }
    }

    public void setVideoMuted(Boolean flag) {
        try {
            //Get client
            //XmlRpcMcuClient client = conf.getMCUClient();
            //Delete participant
            client.SetMute(conf.getId(), id, MediaType.VIDEO, flag);
            //Set it
            videoMuted = flag;
        } catch (XmlRpcException ex) {
            Logger.getLogger(Participant.class.getName()).log(Level.SEVERE, "Failed to mute participant.", ex);
        }
    }

    public void setTextMuted(Boolean flag) {
        try {
            //Get client
            //XmlRpcMcuClient client = conf.getMCUClient();
            //Delete participant
            client.SetMute(conf.getId(), id, MediaType.TEXT, flag);
            //Set it
            textMuted = flag;
        } catch (XmlRpcException ex) {
            Logger.getLogger(Participant.class.getName()).log(Level.SEVERE, "Failed to mute participant.", ex);
        }
    }

    public Boolean getAudioMuted() {
        return audioMuted;
    }

    public Boolean getTextMuted() {
        return textMuted;
    }

    public Boolean getVideoMuted() {
        return videoMuted;
    }

    /**
     * * Must be overrriden by children
     */
    public boolean setVideoProfile(Profile profile) {
        return false;
    }

    public boolean accept() {
        return false;
    }

    public boolean reject(Integer code, String reason) {
        return false;
    }

    public void restart() {
    }

    public void end() {
    }

    public void destroy() {
    }

    void requestFPU() {
    }

    public abstract String getUsername();

    public JSONObject toJSONObject() throws JSONException {
        JSONObject json = new JSONObject();
        json.put("id", getId());
        if (this.getType() == Type.SIP) {
            json.put("name", getDisplayName());
        } else {
            json.put("name", getName());
        }
        json.put("state", getState());
        json.put("ip", getSendIp());
        json.put("admin", isAdmin());
        json.put("ismute", getAudioMuted());
        json.put("rtsp_url", "rtsp://" + conf.getMixer().getPublicIp() + "/" + conf.getId() + getId());
        return json;
    }

    public void AddSpyer(Spyer spyer){
        spyers.put(spyer.getSpyer_number(), spyer);
    }
    public Spyer GetSpyer(Integer spyer){
        return spyers.get(spyer);
    }
}
