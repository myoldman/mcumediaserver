<%-- 
    Document   : rtspViewer
    Created on : 2012-2-15, 21:30:39
    Author     : liuhong
--%>

<%@page import="java.util.Iterator"%>
<%@page contentType="text/html" pageEncoding="UTF-8"%>
<jsp:useBean id="confMngr" class="org.murillo.mcuWeb.ConferenceMngr" scope="application" />
<%
    //Get the conference id
    String uid = request.getParameter("uid");
    //Get conference
    org.murillo.mcuWeb.Conference conf = confMngr.getConference(uid);
    //Check it
    if (conf==null)
	//Go to index
	response.sendRedirect("index.jsp");
    //Get participant iterator
   Iterator<org.murillo.mcuWeb.Participant> itPart = null;
%>
<!DOCTYPE html>
<html>
    <head>
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8">
        <title>会议成员图像</title>
    </head>
    <body>
         <embed type="application/x-vlc-plugin"
         name="video2"
         autoplay="true" loop="no" hidden="no"
         target="rtsp://<%= conf.getMixer().getPublicIp() %>/<%=conf.getId()%>" />
         
           <%
        //Reset iterator
        itPart = conf.getParticipants().values().iterator();
        //Loop 
        while(itPart.hasNext()) {
            // Get participant
           org.murillo.mcuWeb.Participant part = itPart.next();
           if(part.getState() != org.murillo.mcuWeb.Participant.State.CONNECTED)
                            continue;
            //Print values
            %>
            <embed type="application/x-vlc-plugin"
         name="<%=part.getName()%>"
         autoplay="true" loop="no" hidden="no"
         target="rtsp://<%= conf.getMixer().getPublicIp() %>/<%=conf.getId()%><%=part.getId()%>" />
        <%
        }
        %>
        
    </body>
</html>
