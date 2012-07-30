/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ffcs.mcu.pojo;

import javax.servlet.sip.SipURI;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

/**
 *
 * @author liuhong
 */
public class SipEndPoint extends EndPoint {

    private String number;
    private SipURI sipUri;

    public SipEndPoint() {
        this.type = EndPointType.SIP;
    }

    /**
     * @return the number
     */
    public String getNumber() {
        return number;
    }

    /**
     * @param number the number to set
     */
    public void setNumber(String number) {
        this.number = number;
    }

    /**
     * @return the sipUri
     */
    public SipURI getSipUri() {
        return sipUri;
    }

    /**
     * @param sipUri the sipUri to set
     */
    public void setSipUri(SipURI sipUri) {
        this.sipUri = sipUri;
    }

    public JSONObject toJSONObject() throws JSONException {
        JSONObject json = new JSONObject();
        json.put("id", getId());
        json.put("name", getName());
        json.put("state", getState());
        if(getSipUri()!= null) {
            json.put("sipUri", getSipUri().toString());
        }else{
            json.put("sipUri", "");
        }
        return json;
    }
}
