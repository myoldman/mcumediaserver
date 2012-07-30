<%@page import="org.murillo.mcuWeb.Conference"%>
<%@page contentType="text/html"%>
<%@page pageEncoding="UTF-8"%>
<%@page import="java.util.Iterator"%> 
<%@page import="org.murillo.mcuWeb.ConferenceMngr"%>
<%@page import="org.murillo.mcuWeb.Participant"%> 
<%@page import="org.murillo.mcuWeb.ConferenceMngr"%>
<%@page import="org.murillo.mcuWeb.exceptions.ConferenceNotFoundExcetpion"%>
<jsp:useBean id="rtspEndPointManager" class="com.ffcs.mcu.RtspEndPointManager" scope="application" />
<jsp:useBean id="sipEndPointManager" class="com.ffcs.mcu.SipEndPointManager" scope="application" />
<%
    Conference conf;
    //Get conference manager
    ConferenceMngr confMngr = (ConferenceMngr) getServletContext().getAttribute("confMngr");
    //Get the conference id
    String uid = request.getParameter("uid");

    try {
	//Get conference
	conf = confMngr.getConference(uid);
    } catch (ConferenceNotFoundExcetpion ex) {
	//Go to index
	response.sendRedirect("index.jsp");
	//Exit
	return;
    }
    //Get participant iterator
   Iterator<Participant> itPart = null;
%>
<script>
    var uid = "<%=uid%>";
    function removeParticipant(partId)
    {
        var param = {uid:uid, partId:partId};
        return callController("removeParticipant", param);
    }
    function setVideoMute(partId, flag)
    {
        var param = {uid:uid, partId:partId, flag:flag };
        return callController("setVideoMute",param);
    }
    function setAudioMute(partId, flag)
    {
        var param = {uid:uid, partId:partId, flag:flag };
        return callController("setAudioMute",param);
    }
    function changeParticipantProfile(partId,profileId)
    {
	var param = {uid:uid, partId:partId, profileId:profileId };
        return callController("changeParticipantProfile",param);
    }
    function setMosaicSlot(num, id)
    {
        var param = {uid:uid, num:num, id:id };
        return callControllerAsync("setMosaicSlot",param);
    }
</script>
<fieldset style="width:48%;float:right">
    <legend><img src="icons/application_view_tile.png"> 切换会议演讲人</legend>
<!--     <img src="icons/mosaic<%=conf.getCompType()%>.png" style="float:right">-->
     <table class="form">
            <form onSumbit="return false;">
            <% 
                //Get vector with slots positions
                Integer[] slots = conf.getMosaicSlots();
               
                //Print it
                for(int i=0;i<conf.getNumSlots();i++)
                {
            %>
            <tr>
                <td>当前演讲人 <%=i+1%>:</td>
                <td><select name="pos" onchange="setMosaicSlot(<%=i%>,this.value);">
<!--                        <option value="0"  <%=slots[i].equals(0)?"selected":""%>>Free-->
<!--                        <option value="-1" <%=slots[i].equals(-1)?"selected":""%>>Lock-->
                <%
                    //Get iterator
                    itPart = conf.getParticipants().values().iterator();
                    //Options strign
                    String options = new String();
                    //Loop 
                    while(itPart.hasNext()) 
                    {
                        // Get mixer
                        org.murillo.mcuWeb.Participant part = itPart.next();
                        if(part.getState() != Participant.State.CONNECTED)
                            continue;
                        //Print it
                        %><option value="<%=part.getId()%>" <%=slots[i].equals(part.getId())?"selected":""%>><%=part.getName()%><%
                    }
                %>
                    </select>
                </td>     
            </tr>
            <%
                }
            %>
            </form>
        </table>
</fieldset>

<fieldset style="width:48%;">
    <legend><img src="icons/image.png"> 会议信息</legend>
        <table class="form">
            <form method="POST" action="controller/setCompositionType">
            <input type="hidden" name="uid" value="<%=conf.getUID()%>">
            <tr>
                <td>名称:</td>
                <td><%=conf.getName()%></td>
            </tr>
            <tr>
                <td>直拨号码:</td>
                <td><%=conf.getDID()%></td>
            </tr>
            <tr>
                <td>媒体服务器:</td>
                <td><%=conf.getMixer().getName()%></td>
            </tr>
            <tr style="display: none;">
                <td>Composition:</td>
                <td><select name="compType" value="<%=conf.getCompType()%>">
                    <%
                        //Get mosaics
                        java.util.HashMap<Integer,String> mosaics = org.murillo.mcuWeb.MediaMixer.getMosaics();
                        //Get iterator
                        Iterator<java.lang.Integer> itMosaics = mosaics.keySet().iterator();
                        //Loop 
                        while(itMosaics.hasNext()) {
                            //Get key and value
                            Integer k = itMosaics.next();
                            String v = mosaics.get(k);
                            %><option value="<%=k%>" <%=conf.getCompType()==k?"selected":""%> ><%=v%><%
                        }
                    %>
                    </select>
                </td>
            </tr>
            <tr style="display: none;">
                <td>Size</td>
                <td><select name="size" value="<%=conf.getSize()%>">
                <%
                        //Get sizes
                        java.util.HashMap<Integer,String> sizes = org.murillo.mcuWeb.MediaMixer.getSizes();
                        //Get iterator
                        Iterator<java.lang.Integer> itSizes = sizes.keySet().iterator();
                        //Loop 
                        while(itSizes.hasNext()) {
                            //Get key and value
                            Integer k = itSizes.next();
                            String v = sizes.get(k);
                            %><option value="<%=k%>" <%=conf.getSize()==k?"selected":""%> ><%=v%><%
                        }
                    %>
                    </select>
                </td>
            </tr>
            <tr style="display: none;">
                <td>Default profile:</td>
                <td><select name="profileId">
                     <%
                        //Get profiles
                        Iterator<org.murillo.mcuWeb.Profile> itProf = confMngr.getProfiles().values().iterator();
                        //Loop 
                        while(itProf.hasNext()) {
                            // Get mixer
                            org.murillo.mcuWeb.Profile profile = itProf.next();
                            //If it's the selected profile
                            %><option value="<%=profile.getUID()%>" <%=profile.getUID().equals(conf.getProfile().getUID())?"selected":""%>><%=profile.getName()%><%
                        }
                    %>
		    </select>
                </td>
            </tr>
            <tr style="display: none;">
               <td colspan=2>
               <input class="accept" type="submit" value="Change">
               </td>
            </tr>
            </form>
        </table>
</fieldset>

<fieldset style="width:48%;">
    <legend><img src="icons/user_add.png"> 添加会议成员</legend>
    <form method="POST" action="controller/callParticipant">
    <input type="hidden" name="uid" value="<%=uid%>">
     <table class="list">
        <tr>
            <th width="10%">选择</th>
            <th width="10%">ID</th>
            <th width="30%">名称</th>
            <th width="40%">号码</th>
            <th width="10%">状态</th>
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
            <td><% if(temp.getState() == com.ffcs.mcu.pojo.EndPoint.State.IDLE) { %> <input type="checkbox" style="width:auto;margin:0 0 0 0;" name="sipids" value="<%=temp.getId()%>"/> <% } %></td>
            <td><%=temp.getId()%></td>
            <td><%=temp.getName()%></td>
            <td><%=temp.getNumber()%></td>
            <td><%=temp.getState()%></td>
        </tr><%
        }
        %>
    </table>
    <input class="add" type="submit" value="发起邀请">
    </form>
</fieldset>
        
<fieldset style="width:48%;">
    <legend><img src="icons/user_add.png"> 添加摄像头</legend>
    <form method="POST" action="controller/callRtspParticipant">
    <input type="hidden" name="uid" value="<%=uid%>">
        <table class="list">
        <tr>
            <th width="10%">选择</th>
            <th width="10%">ID</th>
            <th width="30%">名称</th>
            <th width="40%">地址</th>
            <th width="10%">状态</th>
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
            <td><% if(temp.getState() == com.ffcs.mcu.pojo.RtspEndPoint.State.IDLE) { %> <input type="checkbox" style="width:auto;margin:0 0 0 0;" name="rtspids" value="<%=temp.getId()%>"/> <% } %></td>
            <td><%=temp.getId()%></td>
            <td><%=temp.getName()%></td>
            <td><%=temp.getRtspUrl()%></td>
            <td><%=temp.getState()%></td>
        </tr><%
        }
        %>
    </table>
    <input class="add" type="submit" value="发起邀请">
    </form>
</fieldset>



<fieldset style="clear:both;">
    <legend><img src="icons/group.png"> 会议成员列表</legend>
        <table class="list">
        <tr>
            <th width="20%">名称</th>
<!--	    <th>Profile</th>-->
            <th width="30%">IP</th>
            <th width="30%">状态</th>
            <th width="20%">操作</th>
        </tr>
        <%
        //Reset iterator
        itPart = conf.getParticipants().values().iterator();
        //Loop 
        while(itPart.hasNext()) {
            // Get participant
           Participant part = itPart.next();
            //Print values
            %>
        <tr>
            <td><%=part.getName()%></td>
<!--	    <td><select name="profileId" onChange="changeParticipantProfile('<%=part.getId()%>',this.value)"><%
                        //Get profiles
                        itProf = confMngr.getProfiles().values().iterator();
                        //Loop
                        while(itProf.hasNext()) {
                            // Get mixer
                            org.murillo.mcuWeb.Profile profile = itProf.next();
                            //If it's the selected profile
                            %><option value="<%=profile.getUID()%>" <%=profile.getUID().equals(part.getVideoProfile().getUID())?"selected":""%>><%=profile.getName()%><%
                        }
                    %></select>
	    </td>-->
            <td><%=part.getSendIp()%></td>
            <td><%=part.getState()%></td>
            <td align="center"> <a target="_blank" href="viewParticipant.jsp?uid=<%=conf.getUID()%>&partId=<%= part.getId() %>"><img src="icons/eye.png"><span>监视成员</span></a>
                <% if (!part.getAudioMuted()) { %><a href="#" onClick="setAudioMute('<%=part.getId()%>',true);return false;"><img src="icons/sound.png"><span>静音</span></a><% } else { %><a href="#" onClick="setAudioMute('<%=part.getId()%>',false);return false;"><img src="icons/sound_mute.png"><span>解除静音</span></a><% } %><% if (!part.getVideoMuted()) { %><a href="#" onClick="setVideoMute('<%=part.getId()%>',true);return false;"><img src="icons/webcam.png"><span>禁止视频</span></a><% } else { %><a href="#" onClick="setVideoMute('<%=part.getId()%>',false);return false;"><img src="icons/webcam_delete.png"><span>打开视频</span></a><% } %><a href="#" onClick="removeParticipant('<%=part.getId()%>');return false;"><img src="icons/bin_closed.png"><span>踢出会议</span></a></td>
        </tr><%
        }
        %>
    </table>
</fieldset>
