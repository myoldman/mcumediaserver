/*
 * ConferenceMngr.java
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
import com.sun.org.apache.xml.internal.serialize.XMLSerializer;
import java.beans.XMLDecoder;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.MalformedURLException;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.UUID;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.ServletContext;
import javax.servlet.sip.*;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.apache.xmlrpc.XmlRpcException;
import org.murillo.MediaServer.Codecs;
import org.murillo.MediaServer.XmlRpcBroadcasterClient;
import org.murillo.MediaServer.XmlRpcEventManager;
import org.murillo.MediaServer.XmlRpcMcuClient;
import org.murillo.mcuWeb.Participant.State;
import org.murillo.mcuWeb.exceptions.ConferenceNotFoundExcetpion;
import org.murillo.mcuWeb.exceptions.ParticipantNotFoundException;
import org.murillo.util.ThreadPool;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NamedNodeMap;
import org.w3c.dom.NodeList;

/**
 * @author Sergio Garcia Murillo
 */
public class ConferenceMngr implements Conference.Listener, XmlRpcEventManager.Listener {

    public void onConnect() {
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "EventManager connected");
    }

    public void onError(String mixerId) {
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "EventManager error");
        //Get the list of conferences
        Iterator<Conference> it = conferences.values().iterator();
        //For each one
        while (it.hasNext()) {
            //Get conference
            Conference conf = it.next();
            //If it  is in the mixer
            if (conf.getMixer().getUID().equals(mixerId)) {
                try {
                    //Remove conference
                    conf.destroy();
                } catch (Exception ex) {
                    //Log
                    Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
        try {
            Thread.sleep(3000);
        } catch (InterruptedException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "sleep error");
        }
        MediaMixer mixer = mixers.get(mixerId);
        if(mixer != null) {
            mixer.disconnectEveneManager();
            mixer.connectEveneManager(this);
        }
    }

    public void onDisconnect() {
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "EventManager disconnected");
    }

    public void onEvent(Object result) {
        Object[] event = (Object[]) result;
        if (event.length == 4) {
            Integer eventType = (Integer) event[0];
            Integer confId = (Integer) event[1];
            Integer partId = (Integer) event[3];
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "EventManager recv event " + eventType);
            Conference conference = searchConferenceById(confId);
            if(conference != null) {
                try {
                    conference.requestFPU(partId);
                } catch (ParticipantNotFoundException ex) {
                    Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }

    public interface Listener {

        void onConferenceCreated(Conference conf);

        void onConferenceDestroyed(String confId);
    };
    private final HashMap<String, MediaMixer> mixers;
    private final ConcurrentHashMap<String, Conference> conferences;
    private final HashMap<String, Broadcast> broadcasts;
    private final HashMap<String, Profile> profiles;
    private final HashMap<String, ConferenceTemplate> templates;
    private String confDir;
    private final HashSet<Listener> listeners;
    private SipFactory sf;
    private int[] confDID;
    private int minConfDID = 5000;
    private int maxConfDID = 30000;

    public ConferenceMngr(ServletContext context) {
        XMLDecoder decoder;

        //Store conf directory
        confDir = context.getRealPath("WEB-INF");

        //If we do not have it
        if (confDir == null) //Empty
        {
            confDir = "";
        } else if (!confDir.endsWith(System.getProperty("file.separator"))) //Add it
        {
            confDir += System.getProperty("file.separator");
        }

        //Get sip factory
        sf = (SipFactory) context.getAttribute(SipServlet.SIP_FACTORY);
        //Create empty maps
        mixers = new HashMap<String, MediaMixer>();
        profiles = new HashMap<String, Profile>();
        templates = new HashMap<String, ConferenceTemplate>();
        conferences = new ConcurrentHashMap<String, Conference>();
        broadcasts = new HashMap<String, Broadcast>();
        listeners = new HashSet<Listener>();

        //Load configurations
        try {
            //Create document builder
            DocumentBuilder builder = DocumentBuilderFactory.newInstance().newDocumentBuilder();

            //Load mixer configuration
            try {
                //Parse document
                Document doc = builder.parse(confDir + "mixers.xml");
                //Get all mixer nodes
                NodeList nodes = doc.getElementsByTagName("mixer");
                //Proccess each mixer
                for (int i = 0; i < nodes.getLength(); i++) {
                    //Get element attributes
                    NamedNodeMap attrs = nodes.item(i).getAttributes();
                    //Get names
                    String name = attrs.getNamedItem("name").getNodeValue();
                    String url = attrs.getNamedItem("url").getNodeValue();
                    String ip = attrs.getNamedItem("ip").getNodeValue();
                    String publicIp = attrs.getNamedItem("publicIp").getNodeValue();
                    String localNet = "";
                    //Check it
                    if (attrs.getNamedItem("localNet") != null) //Get local net value
                    {
                        localNet = attrs.getNamedItem("localNet").getNodeValue();
                    }
                    try {
                        //Create mixer
                        MediaMixer mixer = new MediaMixer(name, url, ip, publicIp, localNet);
                        //Append mixer
                        mixers.put(mixer.getUID(), mixer);
                    } catch (MalformedURLException ex) {
                        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "Invalid URI for mediaserver {0}", name);
                    }
                }
            } catch (Exception ex) {
                Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "Failed to read media mixer conf file: {0}", ex.getMessage());
            }

            //Load profiles configuration
            try {
                //Parse document
                Document doc = builder.parse(confDir + "profiles.xml");
                //Get all mixer nodes
                NodeList nodes = doc.getElementsByTagName("profile");
                //Proccess each mixer
                for (int i = 0; i < nodes.getLength(); i++) {
                    //Get element attributes
                    NamedNodeMap attrs = nodes.item(i).getAttributes();
                    //Create mixer
                    Profile profile = new Profile(
                            attrs.getNamedItem("uid").getNodeValue(),
                            attrs.getNamedItem("name").getNodeValue(),
                            Integer.parseInt(attrs.getNamedItem("videoSize").getNodeValue()),
                            Integer.parseInt(attrs.getNamedItem("videoBitrate").getNodeValue()),
                            Integer.parseInt(attrs.getNamedItem("videoFPS").getNodeValue()),
                            Integer.parseInt(attrs.getNamedItem("intraPeriod").getNodeValue()));
                    //Append mixer
                    profiles.put(profile.getUID(), profile);
                }
            } catch (Exception ex) {
                Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
            }

            //Load templates configuration
            String templateAdHoc = confDir + "templates.xml";
            try {
                //Parse document
                Document doc = builder.parse(templateAdHoc);
                //Get all mixer nodes
                NodeList nodes = doc.getElementsByTagName("template");
                //Proccess each mixer
                for (int i = 0; i < nodes.getLength(); i++) {
                    //Get element attributes
                    NamedNodeMap attrs = nodes.item(i).getAttributes();
                    //Get mixer uid
                    String mixerUID = attrs.getNamedItem("mixer").getNodeValue();
                    String profileUID = attrs.getNamedItem("profile").getNodeValue();
                    //Chek if we have that mixer and profile
                    if (!mixers.containsKey(mixerUID) || !profiles.containsKey(profileUID)) //Skip this
                    {
                        continue;
                    }
                    //Create template
                    ConferenceTemplate template = new ConferenceTemplate(
                            attrs.getNamedItem("name").getNodeValue(),
                            attrs.getNamedItem("did").getNodeValue(),
                            mixers.get(mixerUID),
                            Integer.parseInt(attrs.getNamedItem("size").getNodeValue()),
                            Integer.parseInt(attrs.getNamedItem("compType").getNodeValue()),
                            profiles.get(profileUID),
                            attrs.getNamedItem("audioCodecs").getNodeValue(),
                            attrs.getNamedItem("videoCodecs").getNodeValue(),
                            attrs.getNamedItem("textCodecs").getNodeValue());
                    //Append mixer
                    templates.put(template.getUID(), template);
                }
            } catch (FileNotFoundException ex) {
                Logger.getLogger(ConferenceMngr.class.getName()).log(Level.WARNING, "Could not find config file for ad hoc templates {0}", templateAdHoc);
            } catch (Exception ex) {
                Logger.getLogger(ConferenceMngr.class.getName()).log(Level.WARNING, "Failed to read config file for ad hoc templates", ex);
            }

        } catch (ParserConfigurationException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
        confDID = new int[maxConfDID - minConfDID];
        for (int i = 0; i < maxConfDID - minConfDID - 1; i++) {
            confDID[i] = 0;
        }
    }

    public int getIdelDID() {
        synchronized (this) {
            for (int i = 0; i < maxConfDID - minConfDID - 1; i++) {
                if (confDID[i] == 0) {
                    confDID[i] = 1;
                    return i + minConfDID;
                }
            }
        }
        return 0;
    }

    public void connectEventManager() {
        //Get mixer iterator
        Iterator<MediaMixer> it = mixers.values().iterator();
        //For each one
        while (it.hasNext()) {
            //Get mixer
            MediaMixer mixer = it.next();
            mixer.connectEveneManager(this);
        }
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "connectEventManager");
    }

    public void disconnectEventManager() {
        //Get mixer iterator
        Iterator<MediaMixer> it = mixers.values().iterator();
        //For each one
        while (it.hasNext()) {
            //Get mixer
            MediaMixer mixer = it.next();
            mixer.disconnectEveneManager();
        }
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "disconnectEventManager");
    }

    public void setSipFactory(SipFactory sf) {
        //Store it
        this.sf = sf;
    }

    public void saveMixersConfiguration() {
        try {
            Document doc = DocumentBuilderFactory.newInstance().newDocumentBuilder().newDocument();
            //create the root element
            Element root = doc.createElement("root");
            //Add to document
            doc.appendChild(root);

            //Get mixer iterator
            Iterator<MediaMixer> it = mixers.values().iterator();
            //For each one
            while (it.hasNext()) {
                //Get mixer
                MediaMixer mixer = it.next();
                //create child element
                Element child = doc.createElement("mixer");
                //Set attributes
                child.setAttribute("name", mixer.getName());
                child.setAttribute("url", mixer.getUrl());
                child.setAttribute("ip", mixer.getIp());
                child.setAttribute("publicIp", mixer.getPublicIp());
                child.setAttribute("localNet", mixer.getLocalNet().toString());
                //Append
                root.appendChild(child);
            }

            //Serrialize to file
            XMLSerializer serializer = new XMLSerializer();
            serializer.setOutputCharStream(new java.io.FileWriter(confDir + "mixers.xml"));
            serializer.serialize(doc);

        } catch (IOException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        } catch (ParserConfigurationException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void saveTemplatesConfiguration() {
        try {
            Document doc = DocumentBuilderFactory.newInstance().newDocumentBuilder().newDocument();
            //create the root element
            Element root = doc.createElement("root");
            //Add to document
            doc.appendChild(root);

            //Get mixer iterator
            Iterator<ConferenceTemplate> it = templates.values().iterator();
            //For each one
            while (it.hasNext()) {
                //Get mixer
                ConferenceTemplate template = it.next();
                //create child element
                Element child = doc.createElement("template");
                //Set attributes
                child.setAttribute("name", template.getName());
                child.setAttribute("did", template.getDID());
                child.setAttribute("size", template.getSize().toString());
                child.setAttribute("compType", template.getCompType().toString());
                child.setAttribute("mixer", template.getMixer().getUID());
                child.setAttribute("profile", template.getProfile().getUID());
                //Set codecs
                child.setAttribute("audioCodecs", template.getAudioCodecs());
                child.setAttribute("videoCodecs", template.getVideoCodecs());
                child.setAttribute("textCodecs", template.getTextCodecs());
                //Append
                root.appendChild(child);
            }

            //Serrialize to file
            XMLSerializer serializer = new XMLSerializer();
            serializer.setOutputCharStream(new java.io.FileWriter(confDir + "templates.xml"));
            serializer.serialize(doc);

        } catch (IOException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        } catch (ParserConfigurationException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public void saveProfileConfiguration() {
        try {
            Document doc = DocumentBuilderFactory.newInstance().newDocumentBuilder().newDocument();
            //create the root element
            Element root = doc.createElement("root");
            //Add to document
            doc.appendChild(root);

            //Get mixer iterator
            Iterator<Profile> it = profiles.values().iterator();
            //For each one
            while (it.hasNext()) {
                //Get mixer
                Profile profile = it.next();
                //create child element
                Element child = doc.createElement("profile");
                //Set attributes
                child.setAttribute("uid", profile.getUID());
                child.setAttribute("name", profile.getName());
                child.setAttribute("videoSize", profile.getVideoSize().toString());
                child.setAttribute("videoBitrate", profile.getVideoBitrate().toString());
                child.setAttribute("videoFPS", profile.getVideoFPS().toString());
                child.setAttribute("intraPeriod", profile.getIntraPeriod().toString());
                //Append
                root.appendChild(child);
            }

            //Serrialize to file
            XMLSerializer serializer = new XMLSerializer();
            serializer.setOutputCharStream(new java.io.FileWriter(confDir + "profiles.xml"));
            serializer.serialize(doc);

        } catch (IOException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        } catch (ParserConfigurationException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public Conference createConference(String name, String did, String mixerId, Integer size, Integer compType, String profileId, String audioCodecs, String videoCodecs, String textCodecs) {
        Conference conf = null;
        //Lock conferences
        synchronized (conferences) {
            //First check if a conference already exist with the same DID
            if (searchConferenceByDid(did) != null) {
                //Log error
                Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "Error creating conference DID already present [did:{0}]", did);
                //No conference created
                return null;
            }
        }
        //Get first available mixer
        MediaMixer mixer = mixers.get(mixerId);
        //Check it
        if (mixer == null) {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "Media mixer {0} does not exist.", mixerId);
            //Exit
            return null;
        }

        //Get profile
        Profile profile = profiles.get(profileId);
        //Check it
        if (profile == null) {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "Profile {0} does not exist.", profileId);
            //Exit
            return null;
        }

        try {
            //Create conference object
            conf = new Conference(sf, name, did, mixer, size, compType, profile, false);
            //If got audio codecs
            if (audioCodecs != null && !audioCodecs.isEmpty()) {
                //Clear codecs for audio
                conf.clearSupportedCodec("audio");
                //For each codec
                for (Integer codec : Codecs.getCodecsFromList("audio", audioCodecs)) //Add it
                {
                    conf.addSupportedCodec("audio", codec);
                }
            }
            //If got video codecs
            if (videoCodecs != null && !videoCodecs.isEmpty()) {
                //Clear codecs for audio
                conf.clearSupportedCodec("video");
                //For each codec
                for (Integer codec : Codecs.getCodecsFromList("video", videoCodecs)) //Add it
                {
                    conf.addSupportedCodec("video", codec);
                }
            }
            //If got audio codecs
            if (textCodecs != null && !textCodecs.isEmpty()) {
                //Clear codecs for audio
                conf.clearSupportedCodec("text");
                //For each codec
                for (Integer codec : Codecs.getCodecsFromList("text", textCodecs)) //Add it
                {
                    conf.addSupportedCodec("text", codec);
                }
            }
            //We don't want to call destroy inside the synchronized block
            boolean done = false;
            //Lock conferences
            synchronized (conferences) {
                //Second check if a conference already exist with the same DID
                if (searchConferenceByDid(did) == null) {
                    //Add listener
                    conf.addListener(this);
                    //Save to conferences
                    conferences.put(conf.getUID(), conf);
                    //We are done
                    done = true;
                }
            }
            //if the did was duplicated
            if (!done) {
                //Log error
                Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "Error creating conference DID already present [did:{0}] destroying conference", did);
                //Delete conference
                conf.destroy();
                //Exit
                return null;
            }
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "Created conference {0}", conf.getUID());
            //Launch event
            fireOnConferenceCreatead(conf);
        } catch (XmlRpcException ex) {
            //Log error
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, "Fail to communicate with media mixer [mixerId:{0},error:\"{1}\"]", new Object[]{mixer.getUID(), ex.getMessage()});
            //No conference created
            return null;
        }

        //Return conference
        return conf;
    }

    public Conference createConferenceAdHoc(String did, ConferenceTemplate template) {
        Conference conf = null;
        //Get first available mixer
        MediaMixer mixer = template.getMixer();
        //Get supported codecs
        String audioCodecs = template.getAudioCodecs();
        String videoCodecs = template.getVideoCodecs();
        String textCodecs = template.getTextCodecs();

        try {
            //Create conference object
            conf = new Conference(sf, template.getName(), did, mixer, template.getSize(), template.getCompType(), template.getProfile(), true);
            //Add listener
            conf.addListener(this);
            //If got audio codecs
            if (audioCodecs != null && !audioCodecs.isEmpty()) {
                //Clear codecs for audio
                conf.clearSupportedCodec("audio");
                //For each codec
                for (Integer codec : Codecs.getCodecsFromList("audio", audioCodecs)) //Add it
                {
                    conf.addSupportedCodec("audio", codec);
                }
            }
            //If got video codecs
            if (videoCodecs != null && !videoCodecs.isEmpty()) {
                //Clear codecs for audio
                conf.clearSupportedCodec("video");
                //For each codec
                for (Integer codec : Codecs.getCodecsFromList("video", videoCodecs)) //Add it
                {
                    conf.addSupportedCodec("video", codec);
                }
            }
            //If got audio codecs
            if (textCodecs != null && !textCodecs.isEmpty()) {
                //Clear codecs for audio
                conf.clearSupportedCodec("text");
                //For each codec
                for (Integer codec : Codecs.getCodecsFromList("text", textCodecs)) //Add it
                {
                    conf.addSupportedCodec("text", codec);
                }
            }
            //Lock conferences
            synchronized (conferences) {
                //Save to conferences
                conferences.put(conf.getUID(), conf);
            }
        } catch (XmlRpcException ex) {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Launch event
        fireOnConferenceCreatead(conf);
        //Return
        return conf;
    }

    public void removeConference(String confId) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Finalize conference
        conf.destroy();
        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "Destroyed conference {0}", conf.getUID());
    }

    public Broadcast createBroadcast(String name, String tag, String mixerId) {
        Integer sesId;
        Broadcast bcast = null;
        //Get first available mixer
        MediaMixer mixer = mixers.get(mixerId);
        try {
            XmlRpcBroadcasterClient client = mixer.createBroadcastClient();
            //Create the broadcast session in the media server
            sesId = client.CreateBroadcast(name, tag, new Integer(0), new Integer(0));
            //Create conference object
            bcast = new Broadcast(sesId, name, tag, mixer);

            //Lock conferences
            synchronized (broadcasts) {
                //Save to conferences
                broadcasts.put(bcast.getUID(), bcast);
            }
            //Publish it
            client.PublishBroadcast(sesId, tag);
        } catch (XmlRpcException ex) {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
        return bcast;
    }

    public RTMPUrl addBroadcastToken(String UID) {
        //Get the conference
        Broadcast bcast = broadcasts.get(UID);
        //Get id
        Integer sesId = bcast.getId();
        //Generate uid token
        String token = UUID.randomUUID().toString();
        //Get the mixer
        MediaMixer mixer = bcast.getMixer();
        try {
            XmlRpcBroadcasterClient client = mixer.createBroadcastClient();
            //Add token
            client.AddBroadcastToken(sesId, token);
        } catch (XmlRpcException ex) {
            //Error
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
        //Add token playback url
        return new RTMPUrl("rtmp://" + mixer.getPublicIp() + "/broadcaster", token);
    }

    public RTMPUrl addConferenceToken(String UID) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(UID);
        //Set conference token
        return conf.addConferenceToken();
    }

    public void removeBroadcast(String UID) {
        //Get the conference
        Broadcast conf = broadcasts.get(UID);
        //Get id
        Integer sesId = conf.getId();
        //Remove conference from list
        broadcasts.remove(UID);
        //Get the mixer
        MediaMixer mixer = conf.getMixer();
        //Remove from it
        try {
            XmlRpcBroadcasterClient client = mixer.createBroadcastClient();
            //Stop broadcast
            client.UnPublishBroadcast(sesId);
            //Remove conference
            client.DeleteBroadcast(sesId);
        } catch (XmlRpcException ex) {
            //Error
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    public HashMap<String, MediaMixer> getMediaMixers() {
        return mixers;
    }

    public MediaMixer getMcu(String UID) {
        return mixers.get(UID);
    }

    public ConcurrentHashMap<String, Conference> getConferences() {
        return conferences;
    }

    public HashMap<String, Broadcast> getBroadcasts() {
        return broadcasts;
    }

    public HashMap<String, ConferenceTemplate> getTemplates() {
        return templates;
    }

    public HashMap<String, Profile> getProfiles() {
        return profiles;
    }

    public Conference getConference(String UID) throws ConferenceNotFoundExcetpion {
        //Get conference
        Conference conf = conferences.get(UID);
        //Check if present
        if (conf == null) //Throw exception
        {
            throw new ConferenceNotFoundExcetpion(UID);
        }
        //Return conference
        return conf;
    }

    public Broadcast getBroadcast(String UID) {
        return broadcasts.get(UID);
    }

    public boolean changeParticipantProfile(String uid, Integer partId, String profileId) throws ConferenceNotFoundExcetpion, ParticipantNotFoundException {
        //Get the conference
        Conference conf = getConference(uid);
        //Get new profile
        Profile profile = profiles.get(profileId);
        //Change profile
        return conf.changeParticipantProfile(partId, profile);
    }

    private Conference searchConferenceByDid(String did) {
        //Check all conferences
        for (Conference conf : conferences.values()) {
            //Check did
            if (did.equals(conf.getDID())) //Return conference
            {
                return conf;
            }
        }
        //Nothing found
        return null;
    }

    private Conference searchConferenceById(Integer id) {
        //Check all conferences
        for (Conference conf : conferences.values()) {
            //Check did
            if (id.equals(conf.getId())) //Return conference
            {
                return conf;
            }
        }
        //Nothing found
        return null;
    }

    public Conference fetchConferenceByDID(String did) {
        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINEST, "fetchConferenceByDID for {0}", new Object[]{did});
        //Search already running conference
        Conference conf = searchConferenceByDid(did);
        //Check if found
        if (conf != null) //return it
        {
            return conf;
        }
        //No conference check templates
        for (ConferenceTemplate temp : templates.values()) {
            //Check did
            if (temp.isDIDMatched(did)) //Create conference
            {
                return createConferenceAdHoc(did, temp);
            }
        }
        //No conference available
        return null;
    }

    public Conference getMappedConference(SipURI from, SipURI uri) {
        //Get did
        String did = uri.getUser();

        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINEST, "looking for conference uri={0} did={1}", new Object[]{uri, did});

        //Search it
        return fetchConferenceByDID(did);
    }

    public MediaMixer addMixer(String name, String url, String ip, String publicIp, String localNet) throws MalformedURLException {
        //Create Mixer
        MediaMixer mixer = new MediaMixer(name, url, ip, publicIp, localNet);
        synchronized (mixers) {
            //Append
            mixers.put(mixer.getUID(), mixer);
            mixer.connectEveneManager(this);
            //Stotre configuration
            saveMixersConfiguration();
        }
        //Exit
        return mixer;
    }

    public void removeMixer(String uid) {
        //Get the list of conferences
        Iterator<Conference> it = conferences.values().iterator();
        //For each one
        while (it.hasNext()) {
            //Get conference
            Conference conf = it.next();
            //If it  is in the mixer
            if (conf.getMixer().getUID().equals(uid)) {
                try {
                    //Remove conference
                    conf.destroy();
                } catch (Exception ex) {
                    //Log
                    Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
        synchronized (mixers) {
            //Remove mixer
            MediaMixer mixer = mixers.remove(uid);
            if (mixer != null) {
                mixer.disconnectEveneManager();
            }
            //Stotre configuration
            saveMixersConfiguration();
        }
    }

    public Profile addProfile(String uid, String name, Integer videoSize, Integer videoBitrate, Integer videoFPS, Integer intraPeriod) {
        //Create Profile
        Profile profile = new Profile(uid, name, videoSize, videoBitrate, videoFPS, intraPeriod);

        synchronized (profiles) {
            //Append
            profiles.put(profile.getUID(), profile);
            //Save profiles
            saveProfileConfiguration();
        }

        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "Created profile {0} with size = {1}, {2} fps, bitrate = {3} kb/s intra every {6} frames", new Object[]{name, videoSize, videoFPS, videoBitrate, intraPeriod});
        //Exit
        return profile;
    }

    public void removeProfile(String uid) {
        synchronized (profiles) {
            //Remove profile
            profiles.remove(uid);
            //Save profiles
            saveProfileConfiguration();
        }
    }

    public void addConferenceAdHocTemplate(String name, String did, String mixerId, Integer size, Integer compType, String profileId, String audioCodecs, String videoCodecs, String textCodecs) {
        //Get the mixer
        MediaMixer mixer = mixers.get(mixerId);
        //Get profile
        Profile profile = profiles.get(profileId);
        //Create the template
        ConferenceTemplate template = new ConferenceTemplate(name, did, mixer, size, compType, profile, audioCodecs, videoCodecs, textCodecs);

        synchronized (templates) {
            //Add it to the templates
            templates.put(template.getUID(), template);
            //Save configuration
            saveTemplatesConfiguration();
        }
    }

    void removeConferenceAdHocTemplate(String uid) {
        synchronized (templates) {
            //Remove profile
            templates.remove(uid);
            //Save templates
            saveTemplatesConfiguration();
        }
    }

    Participant callParticipant(String confId, String dest) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //And call the participant
        return conf.callParticipant(dest);
    }

    public boolean acceptParticipant(String confId, Integer partId, Integer mosaicId) throws ConferenceNotFoundExcetpion, ParticipantNotFoundException {
        try {
            //Get the conference
            Conference conf = getConference(confId);
            //And accept the participant
            return conf.acceptParticipant(partId, mosaicId);
        } catch (XmlRpcException ex) {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.SEVERE, null, ex);
            //Error
            return false;
        }
    }

    public void rejectParticipant(String confId, Integer partId) throws ConferenceNotFoundExcetpion, ParticipantNotFoundException {
        //Get the conference
        Conference conf = getConference(confId);
        //And reject the participant
        conf.rejectParticipant(partId);
    }

    public void removeParticipant(String confId, Integer partId) throws ConferenceNotFoundExcetpion, ParticipantNotFoundException {
        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINEST, "removing participant confId={0} partId={1}", new Object[]{confId, partId});
        //Get the conference and participants
        Conference conf = getConference(confId);
        //Let it remove
        conf.removeParticipant(partId);
    }

    Participant callRtspParticipant(String confId, String rtspId, String name, String dest) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //And call the participant
        return conf.callRtspParticipant(rtspId, name, dest);
    }

    public void setCompositionType(String confId, Integer compType, Integer size, String profileId) {
        //Set the composition type for the default mosaic
        setCompositionType(confId, XmlRpcMcuClient.DefaultMosaic, compType, size, profileId);
    }

    public void setCompositionType(String confId, Integer mosaicId, Integer compType, Integer size, String profileId) {
        //Get conference
        Conference conf = conferences.get(confId);
        //Get profile
        Profile profile = profiles.get(profileId);
        //Set values in conference
        conf.setProfile(profile);
        //Set composition
        conf.setCompositionType(mosaicId, compType, size);
    }

    public void setMosaicSlot(String confId, Integer mosaicId, Integer num, Integer partId) {
        //Get conference
        Conference conf = conferences.get(confId);
        //Set values in conference
        conf.setMosaicSlot(mosaicId, num, partId);
    }

    public void setMosaicSlot(String confId, Integer num, Integer partId) {
        //Set the default mosaic slotp
        setMosaicSlot(confId, XmlRpcMcuClient.DefaultMosaic, num, partId);
    }

    public void setAudioMute(String confId, Integer partId, Boolean flag) throws ConferenceNotFoundExcetpion, ParticipantNotFoundException {
        //Get the conference
        Conference conf = getConference(confId);
        //Set mute
        conf.setParticipantAudioMute(partId, flag);
    }

    public void setVideoMute(String confId, Integer partId, Boolean flag) throws ParticipantNotFoundException, ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Set video mute
        conf.setParticipantVideoMute(partId, flag);
    }

    public void onInviteRequest(SipServletRequest request) throws IOException {
        //Create ringing
        SipServletResponse resp = request.createResponse(100, "Trying");
        //Send it
        resp.send();
        //Get called uri
        SipURI uri = (SipURI) request.getRequestURI();
        //Get caller
        SipURI from = (SipURI) request.getFrom().getURI();
        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINEST, "looking for conference for uri={0}", uri);
        //Get default conference
        Conference conf = getMappedConference(from, uri);
        //If not found
        if (conf == null) {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "no such conference for uri={0}", uri);
            //Send 404 response
            request.createResponse(404).send();
            //Exit
            return;
        }
        //Get name
        String name = request.getFrom().getDisplayName();

        //LOg
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.INFO, "found conference confId={0}", conf.getId());
        //If empty
        if (name == null || name.isEmpty() || name.equalsIgnoreCase("anonymous")) //Set to user
        {
            name = from.getUser();
        }
        
        String spy_number = request.getHeader("spy_number");
  
        if(spy_number != null) {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINEST, "creating spy for confId={0} partId = {1}", new Object[]{ conf.getId(), spy_number});
            Spyer spyer = conf.createSpyer(spy_number, from.getUser());
            spyer.onInviteRequest(request);
        } else {
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINEST, "creating participant for confId={0}", conf.getId());

            //Create participant
            RTPParticipant part = (RTPParticipant) conf.createParticipant(Participant.Type.SIP, XmlRpcMcuClient.DefaultMosaic, name);
            String displayName = SipEndPointManager.INSTANCE.getSipEndPointNameByNumber(name);
            if (request.getHeader("desktop_share") != null) {
                displayName = displayName + "-桌面共享";
            }
            if (displayName != null) {
                part.setDisplayName(displayName);
            }
            //Log
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINEST, "calling part.onInvite for confId={0}", conf.getId());
            //Let it handle the request
            part.onInviteRequest(request);
        }
    }

    public void onConferenceEnded(Conference conf) {
        synchronized (this) {
            String did = conf.getDID();
            confDID[Integer.valueOf(did) - minConfDID] = 0;
        }
        //Get id
        String confId = conf.getUID();
        //Remove conference from list
        conferences.remove(confId);
        //fire event
        fireOnConferenceDestroyed(confId);
    }

    public void onParticipantCreated(String confId, Participant part) {
        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINE, "participant created confId={0} partName={1}", new Object[]{confId, part.getName()});
    }

    public void onParticipantStateChanged(String confId, Integer partId, State state, Object data, Participant part) {
        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINE, "participant state changed confId={0} partName={1} state={3}", new Object[]{confId, part.getName(), state});
    }

    public void onParticipantDestroyed(String confId, Integer partId) {
        //Log
        Logger.getLogger(ConferenceMngr.class.getName()).log(Level.FINE, "participant destroyed confId={0} partId={1}", new Object[]{confId, partId});
    }

    public void addListener(Listener listener) {
        //Add it
        listeners.add(listener);
    }

    public void removeListener(Listener listener) {
        //Remove from set
        listeners.remove(listener);
    }

    private void fireOnConferenceDestroyed(final String confId) {
        //Check listeners
        if (listeners.isEmpty()) //Exit
        {
            return;
        }
        //Launch asing
        ThreadPool.Execute(new Runnable() {

            @Override
            public void run() {
                //For each listener in set
                for (Listener listener : listeners) //Send it
                {
                    listener.onConferenceDestroyed(confId);
                }
            }
        });
    }

    private void fireOnConferenceCreatead(final Conference conf) {
        //Check listeners
        if (listeners.isEmpty()) //Exit
        {
            return;
        }
        //Launch asing
        ThreadPool.Execute(new Runnable() {

            @Override
            public void run() {
                //For each listener in set
                for (Listener listener : listeners) //Send it
                {
                    listener.onConferenceCreated(conf);
                }
            }
        });
    }

    public int createMosaic(String confId, Integer compType, Integer size) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Create mosaic
        return conf.createMosaic(compType, size);
    }

    public boolean setMosaicOverlayImage(String confId, Integer mosaicId, String filename) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Create mosaic
        return conf.setMosaicOverlayImage(mosaicId, filename);
    }

    public boolean resetMosaicOverlay(String confId, Integer mosaicId) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Create mosaic
        return conf.resetMosaicOverlay(mosaicId);
    }

    public boolean deleteMosaic(String confId, Integer mosaicId) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Delete mosaic
        return conf.deleteMosaic(mosaicId);
    }

    public void addMosaicParticipant(String confId, Integer mosaicId, Integer partId) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Add mosaic participant
        conf.addMosaicParticipant(mosaicId, partId);
    }

    public void removeMosaicParticipant(String confId, Integer mosaicId, Integer partId) throws ConferenceNotFoundExcetpion {
        //Get the conference
        Conference conf = getConference(confId);
        //Remove mosaic participant
        conf.removeMosaicParticipant(mosaicId, partId);
    }

    public void onConferenceParticipantRequestFPU(MediaMixer mixer, Integer confId, String tag, Integer partId) {
        try {
            //Get conference
            Conference conf = getConference(tag);
            //Send FPU request
            conf.requestFPU(partId);
        } catch (ParticipantNotFoundException ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.WARNING, ex.getMessage());
        } catch (ConferenceNotFoundExcetpion ex) {
            Logger.getLogger(ConferenceMngr.class.getName()).log(Level.WARNING, ex.getMessage());
        }
    }
}