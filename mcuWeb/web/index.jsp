<%@page import="java.text.SimpleDateFormat"%>
<%@page import="java.text.DateFormat"%>
<%@page contentType="text/html"%>
<%@page pageEncoding="UTF-8"%>
<%@page import="java.util.Iterator"%> 
<%@page import="org.murillo.mcuWeb.ConferenceMngr"%>
<%
    //Get conference manager
    ConferenceMngr confMngr = (ConferenceMngr) getServletContext().getAttribute("confMngr");
%>
<jsp:useBean id="rtspEndPointManager" class="com.ffcs.mcu.RtspEndPointManager" scope="application" />
<jsp:useBean id="sipEndPointManager" class="com.ffcs.mcu.SipEndPointManager" scope="application" />
<script>
function removeConf(uid)
{
    var param = {uid:uid};
    return callController("removeConference",param);
}
function removeBroadcast(uid)
{
    var param = {uid:uid};
    return callController("removeBroadcast",param);
}
function removeMixer(uid)
{
    var param = {uid:uid};
    return callController("removeMixer",param);
}
function removeProfile(uid)
{
    var param = {uid:uid};
    return callController("removeProfile",param);
}
function removeConfTemplate(uid)
{
    var param = {uid:uid};
    return callController("removeConferenceAdHocTemplate",param);
}
function removeRtspEndPoint(uid)
{
    var param = {uid:uid};
    return callController("removeRtspEndPoint",param);
}
function removeSipEndPoint(uid)
{
    var param = {uid:uid};
    return callController("removeSipEndPoint",param);
}
</script>    
<fieldset>
    <legend><img src="icons/bricks.png"> 媒体服务器</legend>
    <table class="list">
        <tr>
            <th width="10%">名称</th>
            <th width="25%">地址</th>
	    <th width="15%">媒体IP</th>
	    <th width="15%">私网IP</th>
	    <th width="15%">公网IP</th>
            <th width="10%">状态</th>
            <th width="10%">操作</th>
        </tr>
        <%
        //Get mixer iterator
        Iterator<org.murillo.mcuWeb.MediaMixer> it = confMngr.getMediaMixers().values().iterator();
        //Loop 
        while(it.hasNext()) {
            // Get mixer
           org.murillo.mcuWeb.MediaMixer mm = it.next();
            //Print values
            %>
        <tr>
            <td><%=mm.getName()%></td>
            <td><%=mm.getUrl()%></td>
	    <td><%=mm.getIp()%></td>
	    <td><%=mm.getLocalNet()%></td>
	    <td><%=mm.getPublicIp()%></td>
            <td> 运行中</td>
            <td  align="center"><a href="#" onClick="removeMixer('<%=mm.getUID()%>');return false;"><img src="icons/bin_closed.png"><span>删除</span></a></td>
        </tr><%
        }
        %>
    </table>
    <form><input type="submit" class="add" onClick="document.location.href='addMixer.jsp';return false;" value="添加"></form>    
</fieldset>
<!--
<fieldset>
    <legend><img src="icons/table_multiple.png"> Profile List</legend>
        <table class="list">
        <tr>
	    <th>Id</th>
            <th>Name</th>
            <th>Parameters</th>
            <th>Actions</th>
        </tr>
        <%
        //Get profile
	for( org.murillo.mcuWeb.Profile profile : confMngr.getProfiles().values())
	{
            //Print values
            %>
        <tr>
	    <td><%=profile.getUID()%></td>
            <td><%=profile.getName()%></td>
            <td><%=org.murillo.mcuWeb.MediaMixer.getSizes().get(profile.getVideoSize())%>-<%=profile.getVideoBitrate()%>Kbs <%=profile.getVideoFPS()%>fps  <%=profile.getIntraPeriod()%></td>
            <td>
<a href="#" onClick="removeProfile('<%=profile.getUID()%>');return false;"><img src="icons/bin_closed.png"><span>Remove profile</span></a>
            </td>
        </tr><%
        }
        %>
    </table>
    <form><input class="add" type="button" onClick="document.location.href='addProfile.jsp';return false;" value="Create"></form>
</fieldset>
<%
    //Only if there are available mixers and profiles
    if (confMngr.getMediaMixers().size()>0 && confMngr.getProfiles().size()>0) {
%>
-->
<fieldset>
    <legend><img src="icons/application_view_list.png"> 摄像头</legend>
        <table class="list">
        <tr>
            <th width="10%">ID</th>
            <th width="20%">名称</th>
            <th width="40%">地址</th>
            <th width="20%">状态</th>
            <th width="10%">操作</th>
        </tr>
        <%
        //Get mixer iterator
        Iterator<com.ffcs.mcu.pojo.RtspEndPoint> itRtspEndPoint = rtspEndPointManager.getRtspEndPoints().values().iterator();
        //Loop
        while(itRtspEndPoint.hasNext()) {
            // Get mixer
           com.ffcs.mcu.pojo.RtspEndPoint temp = itRtspEndPoint.next();
            //Print values
            %>
        <tr>
            <td><%=temp.getId()%></td>
            <td><%=temp.getName()%></td>
            <td><%=temp.getRtspUrl()%></td>
            <td><%=temp.getState()%></td>
            <td  align="center">
<a href="#" onClick="removeRtspEndPoint('<%=temp.getId()%>');return false;"><img src="icons/bin_closed.png"><span>删除摄像头</span></a>
            </td>
        </tr><%
        }
        %>
    </table>
    <form><input class="add" type="button" onClick="document.location.href='addRtspEndPoint.jsp';return false;" value="创建摄像头"></form>
</fieldset>
    
<fieldset>
    <legend><img src="icons/application_view_list.png"> 客户端</legend>
        <table class="list">
        <tr>
            <th width="10%">ID</th>
            <th width="20%">名称</th>
            <th width="40%">号码</th>
            <th width="20%">状态</th>
            <th width="10%">操作</th>
        </tr>
        <%
        //Get mixer iterator
        Iterator<com.ffcs.mcu.pojo.SipEndPoint> itSipEndPoint = sipEndPointManager.getSipEndPoints().values().iterator();
        //Loop
        while(itSipEndPoint.hasNext()) {
            // Get mixer
           com.ffcs.mcu.pojo.SipEndPoint temp = itSipEndPoint.next();
            //Print values
            %>
        <tr>
            <td><%=temp.getId()%></td>
            <td><%=temp.getName()%></td>
            <td><%=temp.getNumber()%></td>
            <td><%=temp.getState()%></td>
            <td  align="center">
<a href="#" onClick="removeSipEndPoint('<%=temp.getId()%>');return false;"><img src="icons/bin_closed.png"><span>删除客户端</span></a>
            </td>
        </tr><%
        }
        %>
    </table>
    <form><input class="add" type="button" onClick="document.location.href='addSipEndPoint.jsp';return false;" value="创建客户端"></form>
</fieldset>
<fieldset>
    <legend><img src="icons/application_view_list.png"> 会议模版</legend>
        <table class="list">
        <tr>
            <th width="10%">直拨号码</th>
            <th width="40%">名称</th>
<!--            <th>Profile</th>-->
            <th width="40%">媒体服务器</th>
            <th width="10%">操作</th>
        </tr>
        <%
        //Get mixer iterator
        Iterator<org.murillo.mcuWeb.ConferenceTemplate> itConfTemp = confMngr.getTemplates().values().iterator();
        //Loop
        while(itConfTemp.hasNext()) {
            // Get mixer
           org.murillo.mcuWeb.ConferenceTemplate temp = itConfTemp.next();
            //Print values
            %>
        <tr>
            <td><%=temp.getDID()%></td>
            <td><%=temp.getName()%></td>
<!--            <td><%=temp.getProfile().getName()%></td>-->
            <td><%=temp.getMixer().getName()%></td>
            <td  align="center">
<a href="#" onClick="removeConfTemplate('<%=temp.getUID()%>');return false;"><img src="icons/bin_closed.png"><span>删除模版</span></a>
            </td>
        </tr><%
        }
        %>
    </table>
    <form><input class="add" type="button" onClick="document.location.href='addConferenceAdHocTemplate.jsp';return false;" value="创建模版"></form>
</fieldset>
<fieldset>
    <legend><img src="icons/images.png"> 会议列表</legend>
        <table class="list">
        <tr>
            <th width="10%">ID</th>
	    <th width="20%">创建时间</th>
            <th width="10%">直拨号码</th>
            <th width="20%">成员数</th>
            <th width="10%">名称</th>
            <th width="20%">媒体服务器</th>
            <th width="10%">操作</th>
        </tr>
        <%
        //Get mixer iterator
        Iterator<org.murillo.mcuWeb.Conference> itConf = confMngr.getConferences().values().iterator();
        //Loop 
        while(itConf.hasNext()) {
            // Get mixer
           org.murillo.mcuWeb.Conference conf = itConf.next();
            //Print values
            %>
        <tr>
            <td><%=conf.getId()%></td>
	    <td><%=new SimpleDateFormat("dd/MM/yyyy HH:mm:ss").format(conf.getTimestamp())%></td>
            <td><%=conf.getDID()%></td>
            <td><%=conf.getNumParcitipants()%></td>
            <td><%=conf.getName()%></td>
            <td><%=conf.getMixer().getName()%></td>
            <td align="center">
	    	<a target="_blank" href="viewConference.jsp?uid=<%=conf.getUID()%>"><img src="icons/eye.png"><span>监视会议</span></a>
                <a target="_blank" href="rtspViewer.jsp?uid=<%=conf.getUID()%>"><img src="icons/eye.png"><span>监视会议</span></a>
<!--<a href="viewConference.jsp?uid=<%=conf.getUID()%>"><img src="icons/eye.png"><span>Watch conference</span></a>-->
<a href="conference.jsp?uid=<%=conf.getUID()%>"><img src="icons/zoom.png"><span>会议详细信息</span></a>
<a href="#" onClick="removeConf('<%=conf.getUID()%>');return false;"><img src="icons/bin_closed.png"><span>删除会议</span></a>
            </td>
        </tr><%
        }
        %>
    </table>
    <form><input class="add" type="button" onClick="document.location.href='userCreateConference.jsp';return false;" value="创建会议"></form>
</fieldset>
<!--
<fieldset>
    <legend><img src="icons/film.png"> Broadcast List</legend>
        <table class="list">
        <tr>
            <th>ID</th>
            <th>Tag</th>
            <th>Name</th>
            <th>Mixer</th>
            <th>Actions</th>
        </tr>
        <%
        //Get mixer iterator
        Iterator<org.murillo.mcuWeb.Broadcast> itBcast = confMngr.getBroadcasts().values().iterator();
        //Loop
        while(itBcast.hasNext()) {
            // Get mixer
           org.murillo.mcuWeb.Broadcast bcast = itBcast.next();
            //Print values
            %>
        <tr>
            <td><%=bcast.getId()%></td>
            <td><%=bcast.getTag()%></td>
            <td><%=bcast.getName()%></td>
            <td><%=bcast.getMixer().getName()%></td>
            <td>
<a href="viewBroadcast.jsp?uid=<%=bcast.getUID()%>"><img src="icons/eye.png"><span>Watch broadcast</span></a>
<a href="#" onClick="removeBroadcast('<%=bcast.getUID()%>');return false;"><img src="icons/bin_closed.png"><span>Remove broadcast</span></a>
            </td>
        </tr><%
        }
        %>
    </table>
    <form><input class="add" type="button" onClick="document.location.href='createBroadcast.jsp';return false;" value="Create"></form>
</fieldset>
<%
    }
%>
-->