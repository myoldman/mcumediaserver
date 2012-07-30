/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ffcs.mcu.pojo;

/**
 *
 * @author liuhong
 */
public class EndPoint {

    public enum EndPointType {
        RTSP,
        SIP
    }
    public enum State {

        NOTREGISTER, IDLE, CONNECTING, CONNECTED, ERROR, TIMEOUT, BUSY, DISCONNECTED
    }
    private int id;
    private String name;
    private State state;
    protected EndPointType type;
    /**
     * @return the id
     */
    public int getId() {
        return id;
    }

    /**
     * @param id the id to set
     */
    public void setId(int id) {
        this.id = id;
    }

    /**
     * @return the name
     */
    public String getName() {
        return name;
    }

    /**
     * @param name the name to set
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * @return the state
     */
    public State getState() {
        return state;
    }

    /**
     * @param state the state to set
     */
    public void setState(State state) {
        this.state = state;
    }
}
