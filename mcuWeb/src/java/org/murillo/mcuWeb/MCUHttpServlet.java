/*
 * MCUHttpServlet.java
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

import com.ffcs.mcu.RtspEndPointManager;
import com.ffcs.mcu.SipEndPointManager;
import com.ffcs.mcu.pojo.EndPoint;
import com.ffcs.mcu.pojo.RtspEndPoint;
import com.ffcs.mcu.pojo.SipEndPoint;
import java.io.*;
import java.net.MalformedURLException;
import java.util.jar.Attributes;
import java.util.jar.Manifest;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.sip.SipFactory;
import javax.servlet.sip.SipServlet;
import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;
import org.murillo.MediaServer.XmlRpcEventManager;
import org.murillo.MediaServer.XmlRpcMcuClient;
import org.murillo.mcuWeb.exceptions.ConferenceNotFoundExcetpion;
import org.murillo.mcuWeb.exceptions.ParticipantNotFoundException;

/**
 *
 * @author Sergio Garcia Murillo
 */
public class MCUHttpServlet extends HttpServlet {

    private SipFactory sf;
    private ConferenceMngr confMngr;
    private RtspEndPointManager rtspEndPointManager;
    private SipEndPointManager sipEndPointManager;
    private String path;

    @Override
    public void destroy(){
        confMngr.disconnectEventManager();
    }
    
    @Override
    public void init() throws ServletException {
        //Retreive the servlet context
        ServletContext context = getServletContext();
        //Get path
        path = context.getContextPath();
        //Get the sf
        sf = (SipFactory) context.getAttribute(SipServlet.SIP_FACTORY);
        //Create conference manager
        confMngr = new ConferenceMngr(context);
        confMngr.connectEventManager();
        //Set csip factory
        confMngr.setSipFactory(sf);
        //Set it
        context.setAttribute("confMngr", confMngr);


        rtspEndPointManager = (RtspEndPointManager) context.getAttribute("rtspEndPointManager");
        if (rtspEndPointManager == null) {
            rtspEndPointManager = RtspEndPointManager.INSTANCE;
            //Set it
            context.setAttribute("rtspEndPointManager", rtspEndPointManager);
        }

        sipEndPointManager = (SipEndPointManager) context.getAttribute("sipEndPointManager");
        if (sipEndPointManager == null) {
            sipEndPointManager = SipEndPointManager.INSTANCE;
            //Set it
            context.setAttribute("sipEndPointManager", sipEndPointManager);
        }

        try {
            //Get the input stream
            InputStream inputStream = context.getResourceAsStream("/META-INF/MANIFEST.MF");
            //Read manifest
            Manifest manifest = new Manifest(inputStream);
            //Gett attributes
            Attributes attr = manifest.getMainAttributes();
            //Put them in the application
            context.setAttribute("BuiltDate", attr.getValue("Built-Date"));
            context.setAttribute("SubversionInfo", attr.getValue("Subversion-Info"));
        } catch (IOException ex) {
            Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    // TODO change flag to 0 when conference is removed
    /**
     * Handles the HTTP
     * <code>GET</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     */
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException {
        request.setCharacterEncoding("UTF-8");
        String uri = request.getRequestURI();
        //Get method
        String method = uri.substring(uri.lastIndexOf("/") + 1, uri.length());
        response.setContentType("text/html;charset=UTF-8");
        PrintWriter out = response.getWriter();
        if (method.equals("getRtspEndPoint")) {
            try {
                JSONArray jsonArray = new JSONArray();
                for (RtspEndPoint rtspEndPoint : rtspEndPointManager.getRtspEndPoints().values()) {
                    jsonArray.put(rtspEndPoint.toJSONObject());
                }
                jsonArray.write(out);
            } catch (JSONException ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else if (method.equals("getMediaMixer")) {
            try {
                JSONArray jsonArray = new JSONArray();
                for (MediaMixer mixer : confMngr.getMediaMixers().values()) {
                    jsonArray.put(mixer.toJSONObject());
                }
                jsonArray.write(out);
            } catch (JSONException ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else if (method.equals("getSipEndPoint")) {
            try {
                JSONArray jsonArray = new JSONArray();
                for (SipEndPoint sipEndPoint : sipEndPointManager.getSipEndPoints().values()) {
                    jsonArray.put(sipEndPoint.toJSONObject());
                }
                jsonArray.write(out);
            } catch (JSONException ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else if (method.equals("getSipEndPointIdle")) {
            try {
                JSONArray jsonArray = new JSONArray();
                for (SipEndPoint sipEndPoint : sipEndPointManager.getSipEndPoints().values()) {
                    if (sipEndPoint.getState() == EndPoint.State.IDLE) {
                        jsonArray.put(sipEndPoint.toJSONObject());
                    }
                }
                jsonArray.write(out);
            } catch (JSONException ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else if (method.equals("getRtspEndPointIdle")) {
            try {
                JSONArray jsonArray = new JSONArray();
                for (RtspEndPoint rtspEndPoint : rtspEndPointManager.getRtspEndPoints().values()) {
                    if (rtspEndPoint.getState() == EndPoint.State.IDLE) {
                        jsonArray.put(rtspEndPoint.toJSONObject());
                    }
                }
                jsonArray.write(out);
            } catch (JSONException ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else if (method.equals("getConference")) {
            String userid = request.getParameter("userid");
            try {
                JSONArray jsonArray = new JSONArray();
                for (Conference conference : confMngr.getConferences().values()) {
                    jsonArray.put(conference.toJSONObject(userid));
                }
                jsonArray.write(out);
            } catch (JSONException ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else if (method.equals("getConferenceMember")) {
            String confid = request.getParameter("confid");
            try {
                Conference conference = confMngr.getConference(confid);
                JSONArray jsonArray = new JSONArray();
                for (Participant participant : conference.getParticipants().values()) {
                    jsonArray.put(participant.toJSONObject());
                }
                jsonArray.write(out);
            } catch (ConferenceNotFoundExcetpion ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            } catch (JSONException ex) {
                Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        } else if(method.equals("getLinphoneVersion")) {
                //Store conf directory
            String confDir = getServletContext().getRealPath("/");
            //If we do not have it
            if (confDir == null) //Empty
            {
                confDir = "";
            } else if (!confDir.endsWith(System.getProperty("file.separator"))) //Add it
            {
                confDir += System.getProperty("file.separator");
            }
            BufferedReader   in   =   new   BufferedReader(new   FileReader( confDir + "linphone_version")); 
            if(in != null) {
                String linphone_version = in.readLine();
                out.println(linphone_version);
            } else {
                out.println("3.5.0");
            }
        }

        out.close();
    }

    /**
     * Handles the HTTP
     * <code>POST</code> method.
     *
     * @param request servlet request
     * @param response servlet response
     */
    @Override
    protected void doPost(HttpServletRequest request, HttpServletResponse response)
            throws ServletException, IOException, MalformedURLException {
        request.setCharacterEncoding("UTF-8");
        //Get uri of the request
        String uri = request.getRequestURI();
        //Get method
        String method = uri.substring(uri.lastIndexOf("/") + 1, uri.length());
        try {
            //Depending on the method
            if (method.equals("createConference")) {
                //Get parameters
                String name = request.getParameter("name");
                String did = request.getParameter("did");
                String mixerId = request.getParameter("mixerId");
                String profileId = request.getParameter("profileId");
                Integer compType = Integer.parseInt(request.getParameter("compType"));
                Integer size = Integer.parseInt(request.getParameter("size"));
                String audioCodecs = request.getParameter("audioCodecs");
                String videoCodecs = request.getParameter("videoCodecs");
                String textCodecs = request.getParameter("textCodecs");
                compType = XmlRpcMcuClient.MOSAIC1x1;
                //Call
                Conference conf = confMngr.createConference(name, did, mixerId, size, compType, profileId, audioCodecs, videoCodecs, textCodecs);
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + conf.getUID());
            } else if (method.equals("userCreateConference")) {
                userCreateConference(request, response);
            } else if (method.equals("linphoneCreateConference")) {
                linphoneCreateConference(request, response);
            } else if (method.equals("removeConference")) {
                //Get parameters
                String uid = request.getParameter("uid");
                //Call
                confMngr.removeConference(uid);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("createBroadcast")) {
                //Get parameters
                String name = request.getParameter("name");
                String tag = request.getParameter("tag");
                String mixerId = request.getParameter("mixerId");
                //Call
                Broadcast bcast = confMngr.createBroadcast(name, tag, mixerId);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("removeBroadcast")) {
                //Get parameters
                String uid = request.getParameter("uid");
                //Call
                confMngr.removeBroadcast(uid);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("callParticipant")) {
                //Get parameters
                String uid = request.getParameter("uid");
                String[] sipids = request.getParameterValues("sipids");
                if (sipids != null && sipids.length > 0) {
                    for (int i = 0; i < sipids.length; i++) {
                        String sipid = sipids[i];
                        SipEndPoint sipEndPoint = sipEndPointManager.getSipEndPoint(sipid);
                        if (sipEndPoint != null && sipEndPoint.getSipUri() != null && sipEndPoint.getState() == EndPoint.State.IDLE) {
                            //Call
                            Participant part = confMngr.callParticipant(uid, "sip:" + sipEndPoint.getSipUri().getUser() + "@" + sipEndPoint.getSipUri().getHost() + ":" + sipEndPoint.getSipUri().getPort());
                            part.setDisplayName(sipEndPoint.getName());
                        }
                    }
                }
                //Call
                //Participant part = confMngr.callParticipant(uid, dest);
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + uid);
            } else if (method.equals("callRtspParticipant")) {
                //Get parameters
                String uid = request.getParameter("uid");
                String[] rtspids = request.getParameterValues("rtspids");
                if (rtspids != null && rtspids.length > 0) {
                    for (int i = 0; i < rtspids.length; i++) {
                        String rtspid = rtspids[i];
                        RtspEndPoint rtspEndPoint = rtspEndPointManager.getRtspEndPoint(rtspid);
                        //Call
                        Participant part = confMngr.callRtspParticipant(uid, rtspid, rtspEndPoint.getName(), rtspEndPoint.getRtspUrl());
                        if (part != null) {
                            rtspEndPointManager.setRtspEndPointState(rtspid, RtspEndPoint.State.CONNECTING);
                        }
                    }
                }
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + uid);
            } else if (method.equals("removeParticipant")) {
                //Get parameters
                String uid = request.getParameter("uid");
                Integer partId = Integer.parseInt(request.getParameter("partId"));
                //Remove participant
                confMngr.removeParticipant(uid, partId);
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + uid);
            } else if (method.equals("setVideoMute")) {
                //Get parameters
                String uid = request.getParameter("uid");
                Integer partId = Integer.parseInt(request.getParameter("partId"));
                Boolean flag = Boolean.parseBoolean(request.getParameter("flag"));
                //Call
                confMngr.setVideoMute(uid, partId, flag);
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + uid);
            } else if (method.equals("setAudioMute")) {
                //Get parameters
                String uid = request.getParameter("uid");
                Integer partId = Integer.parseInt(request.getParameter("partId"));
                //Boolean flag = Boolean.parseBoolean(request.getParameter("flag"));
                //Call
                confMngr.setAudioMute(uid, partId, true);
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + uid);
            } else if (method.equals("addProfile")) {
                //Get parameters
                String uid = request.getParameter("uid");
                String name = request.getParameter("name");
                Integer videoSize = Integer.parseInt(request.getParameter("videoSize"));
                Integer videoBitrate = Integer.parseInt(request.getParameter("videoBitrate"));
                Integer videoFPS = Integer.parseInt(request.getParameter("videoFPS"));

                Integer intraPeriod = Integer.parseInt(request.getParameter("intraPeriod"));
                //Call
                confMngr.addProfile(uid, name, videoSize, videoBitrate, videoFPS, intraPeriod);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("removeProfile")) {
                //Get parameters
                String uid = request.getParameter("uid");
                //Call
                confMngr.removeProfile(uid);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("addMixer")) {
                //Get parameters
                String name = request.getParameter("name");
                String url = request.getParameter("url");
                String ip = request.getParameter("ip");
                String publicIp = request.getParameter("publicIp");
                String localNet = request.getParameter("localNet");
                //Call
                confMngr.addMixer(name, url, ip, publicIp, localNet);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("removeMixer")) {
                //Get parameters
                String uid = request.getParameter("uid");
                //Call
                confMngr.removeMixer(uid);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("addConferenceAdHocTemplate")) {
                //Get parameters
                String name = request.getParameter("name");
                String did = request.getParameter("did");
                String mixerId = request.getParameter("mixerId");
                String profileId = request.getParameter("profileId");
                Integer compType = Integer.parseInt(request.getParameter("compType"));
                Integer size = Integer.parseInt(request.getParameter("size"));
                compType = XmlRpcMcuClient.MOSAIC1x1;
                //Get codecs
                String audioCodecs = request.getParameter("audioCodecs");
                String videoCodecs = request.getParameter("videoCodecs");
                String textCodecs = request.getParameter("textCodecs");
                //Call
                confMngr.addConferenceAdHocTemplate(name, did, mixerId, size, compType, profileId, audioCodecs, videoCodecs, textCodecs);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("removeConferenceAdHocTemplate")) {
                //Get parameters
                String uid = request.getParameter("uid");
                //Call
                confMngr.removeConferenceAdHocTemplate(uid);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("addSipEndPoint")) {
                //Get parameters
                String name = request.getParameter("name");
                String number = request.getParameter("number");
                //Call
                sipEndPointManager.addSipEndPoint(name, number);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("removeSipEndPoint")) {
                //Get parameters
                String uid = request.getParameter("uid");
                //Call
                sipEndPointManager.removeSipEndPoint(uid);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("addRtspEndPoint")) {
                //Get parameters
                String name = request.getParameter("name");
                String url = request.getParameter("url");
                //Call
                rtspEndPointManager.addRtspEndPoint(name, url);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("removeRtspEndPoint")) {
                //Get parameters
                String uid = request.getParameter("uid");
                //Call
                rtspEndPointManager.removeRtspEndPoint(uid);
                //Redirect
                response.sendRedirect(path);
            } else if (method.equals("setCompositionType")) {
                //Get parameters
                String uid = request.getParameter("uid");
                Integer compType = Integer.parseInt(request.getParameter("compType"));
                Integer size = Integer.parseInt(request.getParameter("size"));
                String profileId = request.getParameter("profileId");
                //Call
                confMngr.setCompositionType(uid, compType, size, profileId);
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + uid);
            } else if (method.equals("changeParticipantProfile")) {
                //Get parameters
                String uid = request.getParameter("uid");
                Integer partId = Integer.parseInt(request.getParameter("partId"));
                String profileId = request.getParameter("profileId");
                //Call
                confMngr.changeParticipantProfile(uid, partId, profileId);
                //Redirect
                response.sendRedirect(path + "/conference.jsp?uid=" + uid);
            } else if (method.equals("setMosaicSlot")) {
                //Get parameters
                String uid = request.getParameter("uid");
                Integer num = Integer.parseInt(request.getParameter("num"));
                Integer id = Integer.parseInt(request.getParameter("id"));
                //Call
                confMngr.setMosaicSlot(uid, num, id);
                //Set xml response
                response.getOutputStream().print("<result>1</result>");
            } else {
                response.setContentType("text/html;charset=UTF-8");
                PrintWriter out = response.getWriter();
                out.println("<html>");
                out.println("<head>");
                out.println("<title>Servlet MCUHttpServlet</title>");
                out.println("</head>");
                out.println("<body>");
                out.println("<h1>Unknown request [" + method + "]</h1>");
                out.println("</body>");
                out.println("</html>");
                out.close();
            }
        } catch (ConferenceNotFoundExcetpion cex) {
            //Log
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, "failed to run" + method, cex);
            //Redirect home
            response.sendRedirect(path);
        } catch (ParticipantNotFoundException pex) {
            //Log
            Logger.getLogger(Conference.class.getName()).log(Level.SEVERE, "failed to run" + method, pex);
            //Redirect home
            response.sendRedirect(path);
        }
    }

    private void linphoneCreateConference(HttpServletRequest request, HttpServletResponse response) throws IOException {
        String name = request.getParameter("name");
        String did = String.valueOf(confMngr.getIdelDID());
        String mixerId = request.getParameter("mixerId");
        String adminId = request.getParameter("admin");
        String profileId = "medium";
        Integer compType = XmlRpcMcuClient.MOSAIC1x1;
        Integer size = XmlRpcMcuClient.CIF;
        String audioCodecs = request.getParameter("audioCodecs");
        String videoCodecs = request.getParameter("videoCodecs");
        String textCodecs = request.getParameter("textCodecs");
        Conference conf = confMngr.createConference(name, did, mixerId, size, compType, profileId, audioCodecs, videoCodecs, textCodecs);
        String[] selected_id = request.getParameterValues("selected_id");
        if (selected_id != null && selected_id.length > 0 && conf != null) {
            for (int i = 0; i < selected_id.length; i++) {
                String memberid = selected_id[i];
                if (memberid == null || memberid.length() == 0) {
                    continue;
                }
                if (memberid.startsWith("rtsp_")) {
                    String rtspid = memberid.substring(5);
                    RtspEndPoint rtspEndPoint = rtspEndPointManager.getRtspEndPoint(rtspid);
                    //Call
                    Participant part = conf.callRtspParticipant(rtspid, rtspEndPoint.getName(), rtspEndPoint.getRtspUrl());
                    part.setDisplayName(rtspEndPoint.getName());
                    if (part != null) {
                        rtspEndPointManager.setRtspEndPointState(rtspid, RtspEndPoint.State.CONNECTING);
                    }
                }
                if (memberid.startsWith("sip_")) {
                    String sipid = memberid.substring(4);
                    SipEndPoint sipEndPoint = sipEndPointManager.getSipEndPoint(sipid);
                    if (sipEndPoint != null && sipEndPoint.getSipUri() != null && sipEndPoint.getState() == EndPoint.State.IDLE) {
                        Participant part = conf.callParticipant("sip:" + sipEndPoint.getSipUri().getUser() + "@" + sipEndPoint.getSipUri().getHost() + ":" + sipEndPoint.getSipUri().getPort());
                        part.setDisplayName(sipEndPoint.getName());
                        if (adminId != null && adminId.equals(sipid)) {
                            part.setAdmin(true);
                        }
                    }

                }
            }
        }
        PrintWriter out = response.getWriter();
        JSONObject jsonObject = new JSONObject();
        try {
            jsonObject.put("did", did);
            jsonObject.write(out);
        } catch (JSONException ex) {
            Logger.getLogger(MCUHttpServlet.class.getName()).log(Level.SEVERE, null, ex);
        }
        out.close();
    }

    private void userCreateConference(HttpServletRequest request, HttpServletResponse response) throws IOException, NumberFormatException {
        String name = request.getParameter("name");
        String did = String.valueOf(confMngr.getIdelDID());
        String mixerId = request.getParameter("mixerId");
        String profileId = "Medium Quality@512:15:1";
        Integer compType = XmlRpcMcuClient.MOSAIC1x1;
        Integer size = XmlRpcMcuClient.CIF;
        String audioCodecs = request.getParameter("audioCodecs");
        String videoCodecs = request.getParameter("videoCodecs");
        String textCodecs = request.getParameter("textCodecs");
        Conference conf = confMngr.createConference(name, did, mixerId, size, compType, profileId, audioCodecs, videoCodecs, textCodecs);
        String[] selected_id = request.getParameterValues("selected_id");
        if (selected_id != null && selected_id.length > 0) {
            for (int i = 0; i < selected_id.length; i++) {
                String memberid = selected_id[i];
                if (memberid == null || memberid.length() == 0) {
                    continue;
                }
                if (memberid.startsWith("rtsp_")) {
                    String rtspid = memberid.substring(5);
                    RtspEndPoint rtspEndPoint = rtspEndPointManager.getRtspEndPoint(rtspid);
                    //Call
                    Participant part = conf.callRtspParticipant(rtspid, rtspEndPoint.getName(), rtspEndPoint.getRtspUrl());
                    if (part != null) {
                        rtspEndPointManager.setRtspEndPointState(rtspid, RtspEndPoint.State.CONNECTING);
                    }
                }
                if (memberid.startsWith("sip_")) {
                    String sipid = memberid.substring(4);
                    SipEndPoint sipEndPoint = sipEndPointManager.getSipEndPoint(sipid);
                    if (sipEndPoint.getSipUri() != null) {
                        conf.callParticipant("sip:" + sipEndPoint.getSipUri().getUser() + "@" + sipEndPoint.getSipUri().getHost() + ":" + sipEndPoint.getSipUri().getPort());
                    }

                }

            }
        }
        //Redirect
        response.sendRedirect(path + "/conference.jsp?uid=" + conf.getUID());
    }

    /**
     * Returns a short description of the servlet.
     */
    @Override
    public String getServletInfo() {
        return "MCU HTTP Servlet";
    }
}
