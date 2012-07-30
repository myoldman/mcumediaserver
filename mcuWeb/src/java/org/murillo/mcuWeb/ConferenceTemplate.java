/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.murillo.mcuWeb;

/**
 *
 * @author Sergio
 */
public class ConferenceTemplate {


    private String uid;
    private String name;
    private String did;
    private MediaMixer mixer;
    private Integer size;
    private Integer compType;
    private Profile profile;
    private String audioCodecs;
    private String videoCodecs;
    private String textCodecs;

    ConferenceTemplate(String name, String did, MediaMixer mixer, Integer size, Integer compType, Profile profile, String audioCodecs,String videoCodecs,String textCodecs) {
       //Set values
        this.uid = did;
        this.name = name;
        this.did = did;
        this.mixer = mixer;
        this.size = size;
        this.compType = compType;
        this.profile = profile;
        this.audioCodecs = audioCodecs;
        this.videoCodecs = videoCodecs;
        this.textCodecs = textCodecs;
    }

    public String getUID() {
        return uid;
    }

     public Integer getCompType() {
        return compType;
    }

    public String getDID() {
        return did;
    }

    public MediaMixer getMixer() {
        return mixer;
    }

    public String getName() {
        return name;
    }

    public Profile getProfile() {
        return profile;
    }

    public Integer getSize() {
        return size;
    }

    public Boolean isDIDMatched(String did) {
        //Check if we match any
        if (this.did.equals("*"))
            //We do
            return true;
        //Get length
        int len = did.length();
        //First check length
        if (this.did.length()!=len)
            //not matched
            return false;
        //Compare each caracter
        for(int i=0;i<len;i++)
            //They have to be the same or the pattern an X
            if (this.did.charAt(i)!='X' && this.did.charAt(i)!=did.charAt(i))
                //Not matched
                return false;
        //Matched!
        return true;
    }

    String getAudioCodecs() {
        return audioCodecs;
}

    String getVideoCodecs() {
        return videoCodecs;
    }

    String getTextCodecs() {
        return textCodecs;
    }
}
