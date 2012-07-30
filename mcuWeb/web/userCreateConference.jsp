<%@page contentType="text/html"%>
<%@page pageEncoding="UTF-8"%>
<%@page import="java.util.Iterator"%>
<%@page import="java.util.HashMap"%>
<jsp:useBean id="confMngr" class="org.murillo.mcuWeb.ConferenceMngr" scope="application" />
<jsp:useBean id="rtspEndPointManager" class="com.ffcs.mcu.RtspEndPointManager" scope="application" />
<jsp:useBean id="sipEndPointManager" class="com.ffcs.mcu.SipEndPointManager" scope="application" />
<style type="text/css">
    div.centent
    {
        float: left;
        text-align: center;
        margin: 10px;
    }
</style>
<script src="jquery-1.7.min.js" type="text/javascript"></script>
<script type="text/javascript">
    $(document).ready(function() {
        //移到右边
        $("#btnAdd").click(function() {
            //获取选中的选项，删除并追加给对方
            $("#unselected_id option:selected").appendTo("#selected_id");
        });
        //移到左边
        $("#btnRemove").click(function() {
            $("#selected_id option:selected").appendTo("#unselected_id");
        });
        //全部移到右边
        $("#btnAdd_all").click(function() {
            //获取全部的选项,删除并追加给对方
            $("#unselected_id option").appendTo("#selected_id");
        });
        //全部移到左边
        $("#btnRemove_all").click(function() {
            $("#selected_id option").appendTo("#unselected_id");
        });
        //双击选项
        $("#unselected_id").dblclick(function() { //绑定双击事件
            //获取全部的选项,删除并追加给对方
            $("option:selected", this).appendTo("#selected_id"); //追加给对方
        });
        //双击选项
        $("#selected_id").dblclick(function() {
            $("option:selected", this).appendTo("#unselected_id");
        });
    });
</script>
<fieldset>
    <legend><img src="icons/image.png"> 创建会议</legend>
    <form method="POST" action="controller/userCreateConference">
        <table class="form">
            <tr style="display:none;">
                <td>Composition:</td>
                <td><select name="compType">
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
                            %><option value="<%=k%>"><%=v%><%
                        }
                    %></select>
                </td>
            </tr>
            <tr style="display:none;">
                <td>Mosaic size:</td>
                <td><select name="size">
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
                            %><option value="<%=k%>"><%=v%><%
                        }
                    %>
                    </select>
                </td>
            </tr>
             <tr style="display:none;">
                <td>Default profile:</td>
                <td><select name="profileId">
                     <%
                        //Get profiles
                        Iterator<org.murillo.mcuWeb.Profile> itProf = confMngr.getProfiles().values().iterator();
                        //Loop
                        while(itProf.hasNext()) {
                            // Get mixer
                            org.murillo.mcuWeb.Profile profile = itProf.next();
                            %><option value="<%=profile.getUID()%>"><%=profile.getName()%><%
                        }
                    %></select>
                </td>
            </tr>
            <tr>
                <td width="60px">名称:</td>
                <td><input type="text" name="name"></td>
            </tr>
            <tr>
                <td width="60px">媒体服务器:</td>
                <td><select name="mixerId">
                        <%
                            //Get mixers
                            Iterator<org.murillo.mcuWeb.MediaMixer> it = confMngr.getMcus().values().iterator();
                            //Loop
                            while (it.hasNext()) {
                                // Get mixer
                                org.murillo.mcuWeb.MediaMixer mixer = it.next();
                        %><option value="<%=mixer.getUID()%>"><%=mixer.getName()%><%
                            }
                            %>
                    </select></td>
            </tr>
        </table>
        <table class="form">
            <tr>
                <td width="60px">会议成员:</td>
                <td width="200px"> 
                    <select multiple="multiple" id="unselected_id" name="unselected_id" style="width: 100px; height: 160px;">
                        <%
                            //Get mixers
                            Iterator<com.ffcs.mcu.pojo.SipEndPoint> itSip = sipEndPointManager.getSipEndPoints().values().iterator();
                            //Loop
                            while (itSip.hasNext()) {
                                // Get mixer
                                com.ffcs.mcu.pojo.SipEndPoint sipEndPoint = itSip.next();
                        %><option value="sip_<%=sipEndPoint.getId()%>"><%=sipEndPoint.getName()%><%
                              }
                            %>
                            <%
                                //Get mixers
                                Iterator<com.ffcs.mcu.pojo.RtspEndPoint> itRtsp = rtspEndPointManager.getRtspEndPoints().values().iterator();
                                //Loop
                                while (itRtsp.hasNext()) {
                                    // Get mixer
                                    com.ffcs.mcu.pojo.RtspEndPoint rtspEndPoint = itRtsp.next();
                            %><option value="rtsp_<%=rtspEndPoint.getId()%>"><%=rtspEndPoint.getName()%><%
                              }
                            %>
                    </select>

                </td>
                <td align="center">                             
                    <div>
                        <button type="button" id="btnAdd" style="cursor:pointer">></button><br />
                        <button type="button" id="btnAdd_all" style="cursor:pointer">>></button>
                    </div>

                    <div>
                        <button type="button" id="btnRemove" style="cursor:pointer"><</button><br />
                        <button type="button" id="btnRemove_all" style="cursor:pointer"><<</button>
                    </div></td>
                <td width="200px">
                    <select multiple="multiple" id="selected_id" name="selected_id" style="width: 100px; height: 160px;">
                    </select>
                </td>
            </tr>
        </table>
        <input class="accept" type="submit" value="创建">
        <input class="cancel" type="submit" onClick="document.location.href='index.jsp';return false;" value="取消">
    </form>
</fieldset>