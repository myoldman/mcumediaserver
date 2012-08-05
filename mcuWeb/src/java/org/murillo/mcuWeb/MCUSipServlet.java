/*
 * MCUSipServlet.java
 *
 * Copyright (C) 2007  Sergio Garcia Murillo
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope t73hat it will be useful,
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
import com.ffcs.mcu.pojo.EndPoint.State;
import com.ffcs.mcu.pojo.SipEndPoint;
import java.io.IOException;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;
import javax.servlet.sip.*;

/**
 *
 * @author Sergio Garcia Murillo
 */
public class MCUSipServlet extends SipServlet implements SipSessionListener, SipApplicationSessionListener {

    private static final long serialVersionUID = 3978425801979081269L;

    @Override
    protected void doResponse(SipServletResponse resp) throws ServletException, IOException {
        //Super processing
        super.doResponse(resp);
        //Get session
        SipSession session = resp.getSession();
        //Get Participant
        Object obj =  session.getAttribute("user");
        if (obj instanceof Spyer) {
            return;
        }
        RTPParticipant part = (RTPParticipant)obj;
        //If not found
        if (part == null) {
            Logger.getLogger(this.getClass().getName()).log(Level.WARNING, "doResponse without participant [idSession:{0},method:{1},from:{2}]", new Object[]{session.getId(), resp.getMethod(), session.getRemoteParty().toString()});
            //Try from the application session
            part = (RTPParticipant) session.getApplicationSession().getAttribute("user");
            //If not found
            if (part == null) {
                //Log
                Logger.getLogger(this.getClass().getName()).log(Level.WARNING, "doResponse without participant [idAppSession:{0}]", new Object[]{session.getApplicationSession().getId()});
                //exit
                return;
            }
            //Set it to the session also
            session.setAttribute("user", part);
        }
        //Check participant
        if (part == null) //Exit
        {
            return;
        }
        //Check methods
        if (resp.getMethod().equals("INFO")) {
            part.onInfoResponse(resp);
        } else if (resp.getMethod().equals("INVITE")) {
            part.onInviteResponse(resp);
        } else if (resp.getMethod().equals("BYE")) {
            part.onByeResponse(resp);
        } else if (resp.getMethod().equals("CANCEL")) {
            part.onCancelResponse(resp);
        }
    }

    @Override
    protected void doOptions(SipServletRequest request) throws IOException {
        //Get Participant
        RTPParticipant part = (RTPParticipant) request.getSession().getAttribute("user");
        //Check participant
        if (part != null) {
            //Handle it
            part.onOptionsRequest(request);
        } else {
            //Linphone and other UAs may send options before invite to determine public IP
            SipServletResponse response = request.createResponse(200);
            //return it
            response.send();
        }
    }

    @Override
    protected void doInvite(SipServletRequest request) throws IOException {
        if (request.isInitial()) {
            //Retreive the servlet context
            ServletContext context = getServletContext();
            //Get Manager
            ConferenceMngr confMngr = (ConferenceMngr) context.getAttribute("confMngr");
            //Handle it
            confMngr.onInviteRequest(request);
        }
    }

    @Override
    protected void doBye(SipServletRequest request) throws ServletException, IOException {
        Object obj = request.getSession().getAttribute("user");
        if (obj instanceof Spyer) {
            Spyer spy = (Spyer) obj;
            spy.onByeRequest(request);
        } else {
            RTPParticipant part = (RTPParticipant) obj;
            //Check participant
            if (part != null) //Handle it
            {
                part.onByeRequest(request);
            }
        }
    }

    @Override
    protected void doAck(SipServletRequest request) throws ServletException, IOException {
        //Get Participant
        Object obj = request.getSession().getAttribute("user");
        if (obj instanceof Spyer) {
            Spyer spy = (Spyer) obj;
            spy.onAckRequest(request);
        } else {
            RTPParticipant part = (RTPParticipant) obj;
            //Check participant
            if (part != null) //Handle it
            {
                part.onAckRequest(request);
            }
        }
    }

    @Override
    protected void doInfo(SipServletRequest request) throws ServletException, IOException {
        //Get Participant
        Object obj = request.getSession().getAttribute("user");
        if (obj instanceof Spyer) {
            Spyer spy = (Spyer) obj;
            spy.onInfoRequest(request);
        } else {
            RTPParticipant part = (RTPParticipant) obj;
            //Check participant
            if (part != null) //Handle it
            {
                part.onInfoRequest(request);
            }
        }
    }

    @Override
    protected void doCancel(SipServletRequest request) throws ServletException, IOException {
        //Get Participant
        RTPParticipant part = (RTPParticipant) request.getSession().getAttribute("user");
        //Check participant
        if (part != null) //Handle it
        {
            part.onCancelRequest(request);
        }
    }

    @Override
    protected void doRegister(SipServletRequest request) throws ServletException, IOException {
        log("Received register request : " + request);
        Address addr = request.getAddressHeader("Contact");
        SipURI sipURI = (SipURI) addr.getURI();
        System.out.println("onlinebank.SipRegistrationServlet :: "
                + "doRegister called, sipURI = "
                + sipURI.toString());
        if (sipURI == null) {
            return;
        }
        String expiresStr = request.getHeader("Expires");
        if (expiresStr == null) {
            expiresStr = addr.getParameter("expires");
        }
        int expires = (expiresStr != null) ? Integer.parseInt(expiresStr) : 0;
//        
//            Registrations.unregister(sipURI);
//            invalidateOngoingSession(sipURI.getUser());
//        } else {
//            Registrations.register(sipURI);
//        }
        SipEndPointManager sipEndPointManager = SipEndPointManager.INSTANCE;
        SipServletResponse resp = null;
        if (expires == 0) {
            String uid = sipEndPointManager.existNumber(sipURI.getUser());
            if (uid != null) {
                resp = request.createResponse(SipServletResponse.SC_OK);
                sipEndPointManager.setSipEndPointState(uid, State.NOTREGISTER);
                sipEndPointManager.getSipEndPoint(uid).setSipUri(null);
            } else {
                resp = request.createResponse(SipServletResponse.SC_NOT_FOUND);
            }
        } else {
            String uid = sipEndPointManager.existNumber(sipURI.getUser());
            if (uid != null) {
                resp = request.createResponse(SipServletResponse.SC_OK);
                SipEndPoint sipEp = sipEndPointManager.getSipEndPoint(uid);
                resp.addHeader("username", sipEp.getName());
                resp.addHeader("userid", String.valueOf(sipEp.getId()));
                if (sipEp.getState() == State.NOTREGISTER) {
                    sipEp.setState(State.IDLE);
                }
                sipEndPointManager.getSipEndPoint(uid).setSipUri(sipURI);
            } else {
                resp = request.createResponse(SipServletResponse.SC_NOT_FOUND);
            }
        }
        log("Send register response : " + resp);
        resp.send();
        request.getApplicationSession().invalidate();
    }

    @Override
    protected void doSubscribe(SipServletRequest request) throws ServletException, IOException {
        log("Received register request : " + request);
        Address addr = request.getAddressHeader("Contact");
        SipURI sipURI = (SipURI) addr.getURI();
        System.out.println("onlinebank.SipRegistrationServlet :: "
                + "doSubscribe called, sipURI = "
                + sipURI.toString());
        if (sipURI == null) {
            return;
        }
        SipServletResponse resp =
                request.createResponse(SipServletResponse.SC_OK);
        log("Send subscribe response : " + resp);
        resp.send();
        request.getApplicationSession().invalidate();
    }

    public void sessionCreated(SipSessionEvent event) {
        Logger.getLogger(this.getClass().getName()).log(java.util.logging.Level.WARNING, "sessionCreated!");
    }

    public void sessionDestroyed(SipSessionEvent event) {
        //Log it       
        Logger.getLogger(this.getClass().getName()).log(Level.WARNING, "sessionDestroyed!");
    }

    public void sessionReadyToInvalidate(SipSessionEvent event) {
        //Log it
        Logger.getLogger(this.getClass().getName()).log(Level.WARNING, "sessionReadyToInvalidate!");
    }

    public void sessionCreated(SipApplicationSessionEvent sase) {
        Logger.getLogger(this.getClass().getName()).log(java.util.logging.Level.WARNING, "sessionCreated!");
    }

    public void sessionDestroyed(SipApplicationSessionEvent sase) {
        Logger.getLogger(this.getClass().getName()).log(java.util.logging.Level.WARNING, "sessionDestroyed!");
    }

    public void sessionExpired(SipApplicationSessionEvent sase) {
        Logger.getLogger(this.getClass().getName()).log(java.util.logging.Level.WARNING, "sessionExpired!");
        //Get application session
        SipApplicationSession applicationSession = sase.getApplicationSession();

        Object obj = applicationSession.getAttribute("user");
        if (obj instanceof Spyer) {
            Spyer spy = (Spyer) obj;
            spy.onTimeout();
        } else {
            RTPParticipant part = (RTPParticipant) obj;
            //Check participant
            if (part != null) //Handle it
            {
                part.onTimeout();
            }
        }
    }

    public void sessionReadyToInvalidate(SipApplicationSessionEvent sase) {
        Logger.getLogger(this.getClass().getName()).log(java.util.logging.Level.WARNING, "sessionReadyToInvalidate!");
    }
}
