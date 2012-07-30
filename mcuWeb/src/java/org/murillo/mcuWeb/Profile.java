/*
 * Profile.java
 * 
 * Created on 03-oct-2007, 12:46:56
 * 
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.murillo.mcuWeb;

/**
 *
 * @author esergar
 */
public class Profile {

    private String uid;
    private String name;
    private Integer videoBitrate;
    private Integer videoFPS;
    private Integer videoSize;
    private Integer videoQmin;
    private Integer videoQmax;
    private Integer intraPeriod;

    public Profile(String uid, String name, Integer videoSize, Integer videoBitrate, Integer videoFPS, Integer intraPeriod) {
        //Set values
        this.uid = uid;
        this.name = name;
        this.videoBitrate = videoBitrate;
        this.videoSize = videoSize;
        this.videoFPS = videoFPS;
        this.intraPeriod = intraPeriod;
    }
    
    public String getUID() {
        return uid;
    }
    
    public String getName() {
        return name;
    }

    public Integer getVideoBitrate() {
        return videoBitrate;
    }

    public Integer getVideoFPS() {
        return videoFPS;
    }

    public Integer getVideoSize() {
        return videoSize;
    }

    public Integer getIntraPeriod() {
        return intraPeriod;
}
}
