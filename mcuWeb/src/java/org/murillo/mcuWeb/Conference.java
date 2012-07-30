/*
 * Conference.java
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
import com.ffcs.mcu.pojo.SipEndPoint;
import java.text.SimpleDateFormat;
import java.util.Map.Entry;
import java.util.*;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.sip.Address;
import javax.servlet.sip.SipFactory;
import javax.servlet.sip.SipURI;
import org.apache.xmlrpc.XmlRpcException;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import org.murillo.MediaServer.Codecs;
import org.murillo.MediaServer.XmlRpcMcuClient;
import org.murillo.MediaServer.XmlRpcMcuClient.MediaStatistics;
import org.murillo.mcuWeb.Participant.State;
import org.murillo.mcuWeb.exceptions.ParticipantNotFoundException;
import org.murillo.util.ThreadPool;

/**
 *
 * @author esergar
 */
public class Conference implements Participant.Listener {

    public interface Listener {

        public void onConferenceEnded(Conference conf);

        public void onParticipantCreated(String confId, Participant part);

        public void onParticipantStateChanged(String confId, Integer partId, State state, Object data, Participant part);

        public void onParticipantDestroyed(String confId, Integer partId);
    }
    protected Integer id;
    protected String UID;
    protected Date timestamp;
    protected String name;
    protected String did;
    protected XmlRpcMcuClient client;
    protected MediaMixer mixer;
    protected ConcurrentHashMap<Integer, Participant> participants;
    protected int numActParticipants;
    protected Integer compType;
    protected Integer size;
    protected Integer numSlots;
    protected Integer slots[];
    protected Profile profile;
    protected Boolean isAdHoc;
    protected SipFactory sf;
    protected boolean addToDefaultMosaic;
    protected boolean isDestroying;
    protected boolean autoAccept;
    protected HashSet<Listener> listeners;
    protected HashMap<String, List<Integer>> supportedCodecs = null;

    /**
     * Creates a new instance of Conference
     */
    public Conference(SipFactory sf, String name, String did, MediaMixer mixer, Integer size, Integer compType, Profile profile, Boolean isAdHoc) throws XmlRpcException {
        //Create the client
        this.client = mixer.createMcuClient();
        //Generate a uuid
        UID = UUID.randomUUID().toString();
        //Create conference
        this.id = client.CreateConference(UID, mixer.getQueueId());
        // for orginal 
        UID = this.id + "@" + mixer.getName();
        //Get timestamp
        this.timestamp = new Date();
        //Save values
        this.mixer = mixer;
        this.name = name;
        this.did = did;
        this.profile = profile;
        this.isAdHoc = isAdHoc;
        //Create listeners
        listeners = new HashSet<Listener>();
        //Default composition and size
        this.compType = compType;
        this.size = size;
        this.numSlots = XmlRpcMcuClient.getMosaicNumSlots(compType);
        //Num of active participants
        this.numActParticipants = 0;
        //Store sip factory
        this.sf = sf;
        //Create conference slots
        this.slots = new Integer[numSlots];
        //Empty
        for (int i = 0; i < numSlots; i++) //All free
        {
            slots[i] = 0;
        }
        //Create supported codec map
        supportedCodecs = new HashMap<String, List<Integer>>();
        //Enable all audio codecs
        addSupportedCodec("audio", Codecs.PCMU);
        addSupportedCodec("audio", Codecs.PCMA);
        addSupportedCodec("audio", Codecs.GSM);
        addSupportedCodec("audio", Codecs.SPEEX16);
        //Enable all video codecs
        addSupportedCodec("video", Codecs.H264);
        addSupportedCodec("video", Codecs.H263_1998);
        addSupportedCodec("video", Codecs.H263_1996);
        //Enable all text codecs
        addSupportedCodec("text", Codecs.T140RED);
        addSupportedCodec("text", Codecs.T140);
        //Create the participant map
        participants = new ConcurrentHashMap<Integer, Participant>();
        //Set composition type
        client.SetCompositionType(id, XmlRpcMcuClient.DefaultMosaic, compType, size);
        //Start broadcast
        client.StartBroadcaster(id);
        //By default add to default mosaic
        addToDefaultMosaic = true;
        //By default do autoaccept
        autoAccept = true;
        //Not isDestroying
        isDestroying = false;
    }

    public void destroy() {
        //Check if already was destroying
        //Logger.getLogger("global").log(Level.SEVERE, "destroy conf");
         synchronized( this) {
            if (isDestroying) //Do nothing
            {
                return;
            }
            //We are isDestroying
            isDestroying = true;
        }
        //Get the list of participants
        Iterator<Participant> it = participants.values().iterator();
        //For each one
        while (it.hasNext()) {
            try {
                //Get the participant
                Participant part = it.next();
                //Disconnect
                part.end();
            } catch (Exception ex) {
                Logger.getLogger("global").log(Level.SEVERE, "Error ending or destroying participant on conference destroy", ex);
            }
        }

        try {
            //Stop broadcast
            client.StopBroadcaster(id);
        } catch (Exception ex) {
            Logger.getLogger("global").log(Level.SEVERE, "Error StopBroadcaster on conference destroy", ex);
        }

        try {
            //Remove conference
            client.DeleteConference(id);
        } catch (Exception ex) {
            Logger.getLogger("global").log(Level.SEVERE, "Error deleting conference on destroy", ex);
        }
        //launch event
        fireOnConferenceEnded();
        //Remove client
        mixer.releaseMcuClient(client);
    }

    public Date getTimestamp() {
        return timestamp;
    }

    public Integer getCompType() {
        return compType;
    }

    public boolean isAutoAccept() {
        return autoAccept;
    }

    public void setAutoAccept(boolean autoAccept) {
        this.autoAccept = autoAccept;
    }

    private void setCompType(Integer compType) {
        //Save mosaic info
        this.compType = compType;

        //Get new number of slots
        Integer n = XmlRpcMcuClient.getMosaicNumSlots(compType);
        //Create new slot array
        Integer[] s = new Integer[n];

        //Copy the old ones into the new one
        for (int i = 0; i < n && i < numSlots; i++) //Copy
        {
            s[i] = slots[i];
        }

        //The rest are empty
        for (int i = numSlots; i < n; i++) //Free slots
        {
            s[i] = 0;
        }

        //Save new slots
        numSlots = n;
        slots = s;
    }

    public Integer getSize() {
        return size;
    }

    public Integer getNumSlots() {
        return numSlots;
    }

    public Integer getNumParcitipants() {
        return numActParticipants;
    }

    public void setSize(Integer size) {
        this.size = size;
    }

    public ConcurrentHashMap<Integer, Participant> getParticipants() {
        return participants;
    }

    public Participant getParticipant(Integer partId) throws ParticipantNotFoundException {
        //Get participant
        Participant part = participants.get(partId);
        //If not found
        if (part == null) //Throw new exception
        {
            throw new ParticipantNotFoundException(partId);
        }
        //return participant
        return part;
    }

    public boolean getAutoAccept() {
        return autoAccept;
    }

    public Integer getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    public String getUID() {
        return UID;
    }

    public String getDID() {
        return did;
    }

    public MediaMixer getMixer() {
        return mixer;
    }

    public Profile getProfile() {
        return profile;
    }

    public void setProfile(Profile profile) {
        this.profile = profile;
    }

    public void setMosaicSlot(Integer num, Integer partId) {
        //Set mosaic slot for default mosaic
        setMosaicSlot(XmlRpcMcuClient.DefaultMosaic, num, partId);
    }

    public void setMosaicSlot(Integer mosaicId, Integer num, Integer partId) {
        //Set composition type
        try {
            //Set mosaic
            client.SetMosaicSlot(id, mosaicId, num, partId);
            //Set slot
            slots[num] = partId;
        } catch (XmlRpcException ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void addMosaicParticipant(Integer mosaicId, Integer partId) {
        try {
            //Add participant to mosaic
            client.AddMosaicParticipant(id, mosaicId, partId);
        } catch (XmlRpcException ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void removeMosaicParticipant(Integer mosaicId, Integer partId) {
        try {
            //Remove participant from mosaic
            client.RemoveMosaicParticipant(id, mosaicId, partId);
        } catch (XmlRpcException ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public Integer[] getMosaicSlots() {
        //Return a clone
        return slots.clone();
    }

    public Boolean isAdHoc() {
        return isAdHoc;
    }

    public Participant createParticipant(Participant.Type type, int mosaicId, String name) {
        Participant part = null;
        Integer partId = -1;

        try {
            //Check name
            if (name == null) //Empte name
            {
                name = "";
            }
            //Create participant in mixer conference
            partId = client.CreateParticipant(id, mosaicId, name.replace('.', '_'), type.valueOf());
            //Check type
            if (type == Participant.Type.SIP) {
                //Create the participant
                part = new RTPParticipant(partId, name, mosaicId, this);
                for (Entry<String, List<Integer>> media : supportedCodecs.entrySet()) //for each codec
                {
                    for (Integer codec : media.getValue()) //Add media codec
                    {
                        ((RTPParticipant) part).addSupportedCodec(media.getKey(), codec);
                    }
                }
            } else {
                part = new RtspParticipant(partId, name, mosaicId, this);
                for (Entry<String, List<Integer>> media : supportedCodecs.entrySet()) //for each codec
                {
                    for (Integer codec : media.getValue()) //Add media codec
                    {
                        ((RTPParticipant) part).addSupportedCodec(media.getKey(), codec);
                    }
                }
            }

            //Set autoaccept
            part.setAutoAccept(autoAccept);
            //Append to list
            participants.put(partId, part);
            //Fire event
            fireOnParticipantCreated(part);
            //Set listener
            part.setListener(this);
        } catch (XmlRpcException ex) {
            Logger.getLogger("global").log(Level.SEVERE, "Failed to create participant: {0}", ex.getMessage());
        }
        return part;
    }

    public int createMosaic(Integer compType, Integer size) {
        try {
            //Create new mosaic
            return client.CreateMosaic(id, compType, size);
        } catch (XmlRpcException ex) {
            //Log
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
            //Error
            return -1;
        }
    }

    public boolean setMosaicOverlayImage(Integer mosaicId, String fileName) {
        try {
            // create new mosaic
            return client.SetMosaicOverlayImage(id, mosaicId, fileName);
        } catch (XmlRpcException ex) {
            //Log
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, "failed to send MCU setMosaicOverlayImage", ex);
            //Exit
            return false;
        }
    }

    public boolean resetMosaicOverlay(Integer mosaicId) {
        try {
            return client.ResetMosaicOverlay(id, mosaicId);
        } catch (XmlRpcException ex) {
            //Log
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, "failed to send MCU ResetMosaicOverlay", ex);
            //exit
            return false;
        }
    }

    public boolean deleteMosaic(Integer mosaicId) {
        try {
            return client.DeleteMosaic(id, mosaicId);
        } catch (XmlRpcException ex) {
            //Log
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, "failed to send MCU DeleteMosaic", ex);
            //Error
            return false;
        }
    }

    public void joinParticipant(Participant part) throws XmlRpcException {
        //Get ids for conference and participant
        Integer partId = part.getId();
        //If auto add participant to default mosaic
        if (addToDefaultMosaic) //Add it to the default mosiac
        {
            client.AddMosaicParticipant(id, XmlRpcMcuClient.DefaultMosaic, partId);
        }
    }

    boolean acceptParticipant(Integer partId, Integer mosaicId) throws XmlRpcException, ParticipantNotFoundException {
        //Get the participant
        Participant part = getParticipant(partId);

        //Set mosaic for participant
        client.SetParticipantMosaic(id, partId, mosaicId);

        //Accept participant
        if (!part.accept()) //Exit
        {
            return false;
        }

        //Join it
        joinParticipant(part);
        //ok
        return true;
    }

    boolean rejectParticipant(Integer partId) throws ParticipantNotFoundException {
        //Get the participant
        Participant part = getParticipant(partId);
        //Reject it
        return part.reject(486, "Rejected");
    }

    public boolean changeParticipantProfile(Integer partId, Profile profile) throws ParticipantNotFoundException {
        //Get the participant
        Participant part = getParticipant(partId);
        //Set new video profile
        return part.setVideoProfile(profile);
    }

    public void setParticipantAudioMute(Integer partId, Boolean flag) throws ParticipantNotFoundException {
        //Get the participant
        Participant part = getParticipant(partId);
        //Set new video profile
        //Depending on the state
        if (part.getAudioMuted()) //UnMute
        {
            part.setAudioMuted(false);
        } else //Mute
        {
            part.setAudioMuted(true);
        }
    }

    public void setParticipantVideoMute(Integer partId, Boolean flag) throws ParticipantNotFoundException {
        //Get the participant
        Participant part = getParticipant(partId);
        //Set new video profile
        part.setVideoMuted(flag);
    }

    void setCompositionType(Integer compType, Integer size) {
        //Set it with default mosaic
        setCompositionType(XmlRpcMcuClient.DefaultMosaic, compType, size);
    }

    void setCompositionType(Integer mosaicId, Integer compType, Integer size) {
        try {
            //Set composition
            client.SetCompositionType(id, mosaicId, compType, size);
            //Set composition size
            setCompType(compType);
            //Set mosaic size
            setSize(size);
        } catch (XmlRpcException ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public RTMPUrl addConferenceToken() {
        //Generate uid token
        String token = UUID.randomUUID().toString();
        try {
            //Add token
            client.AddConferencetToken(id, token);
        } catch (XmlRpcException ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Add token playback url
        return new RTMPUrl("rtmp://" + mixer.getPublicIp() + "/mcu/" + id + "/watcher", token);
    }

    synchronized Participant  callParticipant(String dest) {
        //Get ip address
        String domain = System.getProperty("SipBindAddress");
        //If it is not set
        if (domain == null) //Set dummy domain
        {
            domain = "mcuWeb";
        }
        //Call
        return callParticipant(dest, "sip:" + did + "@" + domain, XmlRpcMcuClient.DefaultMosaic);
    }

    Participant callRtspParticipant(String rtspId, String name, String dest) {
        RtspParticipant part = null;
        try {
            //Get name
            int mosaicId = XmlRpcMcuClient.DefaultMosaic;
            //If empty
            if (name == null || name.isEmpty() || name.equalsIgnoreCase("anonymous")) {
                name = "anonymous";
            }
            //Create participant
            part = (RtspParticipant) createParticipant(Participant.Type.RTSP, mosaicId, name);
            //Make call
            part.doConnect(dest, name, rtspId);
        } catch (Exception ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Return participant
        return part;
    }

    Participant callParticipant(String dest, String orig, int mosaicId) {

        try {
            //Create addresses
            Address to = sf.createAddress(dest);
            Address from = sf.createAddress(orig);
            //Get name
            String partName = to.getDisplayName();
            //If empty
            if (partName == null || partName.isEmpty() || partName.equalsIgnoreCase("anonymous")) //Set to user
            {
                partName = ((SipURI) to.getURI()).getUser();
            }
            //Create participant
            RTPParticipant part = (RTPParticipant) createParticipant(Participant.Type.SIP, mosaicId, partName);
            //Make call
            part.doInvite(sf, from, to);
            //Return participant
            return part;
        } catch (Exception ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Return participant
        return null;
    }

    void removeParticipant(Integer partId) throws ParticipantNotFoundException {
        //Get participant
        Participant part = getParticipant(partId);
        //Log
        Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, "removing participant partId={0} partName={1}", new Object[]{partId, part.getName()});
        //End it
        part.end();
    }

    Map<String, MediaStatistics> getParticipantStats(Integer partId) {
        try {
            //Get them
            return client.getParticipantStatistics(id, partId);
        } catch (XmlRpcException ex) {
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, null, ex);
            return null;
        }
    }

    public void onStateChanged(Participant part, State state) {
        //If we are destroying
        if (isDestroying) //Exit
        {
            return;
        }
        //Launch event
        fireOnParticipantStateChanged(part, state, null);
        //Check previous state
        if (state.equals(State.CONNECTING) || state.equals(State.WAITING_ACCEPT)) {
            //Increase the number of active participanst
            numActParticipants++;
        }

        if (state == State.CONNECTED) {
            for (Participant partIter : participants.values()) {
                if (partIter.getType() == Participant.Type.SIP && partIter.getState() == State.CONNECTED) {
                    ((RTPParticipant)partIter).sendMemberNofity(state, part.getId());
                }
            }
            if(part.is_desktop_share) {
                setMosaicSlot(0, part.getId());
            }
        }

        if (state == State.DISCONNECTED) {
            for (Participant partIter : participants.values()) {
                if (partIter.getType() == Participant.Type.SIP && partIter.getState() == State.CONNECTED) {
                    ((RTPParticipant)partIter).sendMemberNofity(state, part.getId());
                }
            }
        }
        //Check new state
        if (state.equals(State.DESTROYED)) {
            //launch ended
            fireOnParticipantDestroyed(part);
            //Reduce number of active participants
            numActParticipants--;
            //Delete
            participants.remove(part.getId());
            //Check if it is an add hoc conference and there are not any other participant
            if (isAdHoc && numActParticipants == 0 && !isDestroying) //We are finished
            {
                destroy();
            }
        }
    }

    public final void clearSupportedCodec(String media) {
        //Check if we have the media
        if (supportedCodecs.containsKey(media)) //clear it
        {
            supportedCodecs.get(media).clear();
        }
    }

    public final void addSupportedCodec(String media, Integer codec) {
        //Check if we have the media
        if (!supportedCodecs.containsKey(media)) //Create it
        {
            supportedCodecs.put(media, new Vector<Integer>());
        }
        //Add codec to media
        supportedCodecs.get(media).add(codec);
    }
    /*
    protected final XmlRpcMcuClient getMCUClient() {
        return client;
    }
    * 
    */

    protected final String getRTPIp() {
        return mixer.getIp();
    }

    protected final String getRTMPIp() {
        return mixer.getPublicIp();
    }

    public void addListener(Listener listener) {
        //Add it
        listeners.add(listener);
    }

    public void removeListener(Listener listener) {
        //Remove from set
        listeners.remove(listener);
    }

    private void fireOnConferenceEnded() {
        //Check listeners
        if (listeners.isEmpty()) //Exit
        {
            return;
        }
        //Store conference
        final Conference conf = this;
        //Launch asing
        ThreadPool.Execute(new Runnable() {

            @Override
            public void run() {
                //For each listener in set
                for (Listener listener : listeners) //Send it
                {
                    listener.onConferenceEnded(conf);
                }
            }
        });

    }

    private void fireOnParticipantCreated(final Participant part) {
        //Check listeners
        if (listeners.isEmpty()) //Exit
        {
            return;
        }
        //Store conference
        final Conference conf = this;
        //Launch asing
        ThreadPool.Execute(new Runnable() {

            @Override
            public void run() {
                //For each listener in set
                for (Listener listener : listeners) //Send it
                {
                    listener.onParticipantCreated(conf.getUID(), part);
                }
            }
        });

    }

    private void fireOnParticipantDestroyed(final Participant part) {
        //Check listeners
        if (listeners.isEmpty()) //Exit
        {
            return;
        }
        //Store conference
        final Conference conf = this;
        //Launch asing
        ThreadPool.Execute(new Runnable() {

            @Override
            public void run() {
                //For each listener in set
                for (Listener listener : listeners) //Send it
                {
                    listener.onParticipantDestroyed(conf.getUID(), part.getId());
                }
            }
        });

    }

    private void fireOnParticipantStateChanged(final Participant participant, final State state, final Object data) {
        //Check listeners
        if (listeners.isEmpty()) //Exit
        {
            return;
        }
        //Store conference
        final Conference conf = this;
        //Launch asing
        ThreadPool.Execute(new Runnable() {

            @Override
            public void run() {
                //For each listener in set
                for (Listener listener : listeners) //Send it
                {
                    listener.onParticipantStateChanged(conf.getUID(), participant.getId(), state, data, participant);
                }
            }
        });
    }

    void requestFPU(Integer partId) throws ParticipantNotFoundException {
        //Get participant
        Participant part = getParticipant(partId);
        //Send request
        part.requestFPU();
    }

    private boolean isUserInConf(String userid) {
        SipEndPoint sipEp = SipEndPointManager.INSTANCE.getSipEndPoint(userid);
        if (sipEp != null) {
            for (Participant part : participants.values()) {
                if (part.getState() == State.CONNECTED && part.getType() == Participant.Type.SIP && part.getUsername().equals(sipEp.getNumber())) {
                    return true;
                }
            }
        }
        return false;
    }

    public void changeAdmin(int partId) {
        for (Participant part : participants.values()) {
            if (part.getId() != partId && part.getState() == State.CONNECTED) {
                part.setAdmin(true);
                break;
            }
        }
    }

    private boolean isAdmin(String userid) {
        SipEndPoint sipEp = SipEndPointManager.INSTANCE.getSipEndPoint(userid);
        if (sipEp != null) {
            for (Participant part : participants.values()) {
                if (part.getType() == Participant.Type.SIP && part.getUsername().equals(sipEp.getNumber())) {
                    return part.isAdmin();
                }
            }
        }
        return false;
    }

    public JSONObject toJSONObject(String userid) throws JSONException {
        JSONObject json = new JSONObject();
        json.put("id", getId());
        json.put("UID", getUID());
        json.put("name", getName());
        json.put("create_time", new SimpleDateFormat("dd/MM/yyyy HH:mm:ss").format(getTimestamp()));
        json.put("member_count", getNumParcitipants());
        json.put("server", getMixer().getName());
        json.put("did", getDID());
        json.put("rtsp_url", "rtsp://" + getMixer().getPublicIp() + "/" + getId());
        if (userid != null && isUserInConf(userid)) {
            json.put("is_in_conf", true);
        } else {
            json.put("is_in_conf", false);
        }

        if (userid != null) {
            json.put("admin", isAdmin(userid));
        }
        return json;
    }
}
