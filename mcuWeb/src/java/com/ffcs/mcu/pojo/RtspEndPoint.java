/*
 * Plain old java object store infomation of rtsp endpoint
 * and open the template in the editor.
 */
package com.ffcs.mcu.pojo;

import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

/**
 *
 * @author liuhong
 */
public class RtspEndPoint extends EndPoint {

    public RtspEndPoint() {
        this.type = EndPointType.RTSP;
    }
    private String rtspUrl;

    /**
     * @return the rtspUrl
     */
    public String getRtspUrl() {
        return rtspUrl;
    }

    /**
     * @param rtspUrl the rtspUrl to set
     */
    public void setRtspUrl(String rtspUrl) {
        this.rtspUrl = rtspUrl;
    }

    public JSONObject toJSONObject() throws JSONException {
        JSONObject json = new JSONObject();
        json.put("id", getId());
        json.put("name", getName());
        json.put("rtspUrl", getRtspUrl());
        return json;
    }
}
