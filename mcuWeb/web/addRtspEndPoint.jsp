<%@page contentType="text/html"%>
<%@page pageEncoding="UTF-8"%>
<%@page import="java.util.Vector"%> 
<jsp:useBean id="rtspEndPointManager" class="com.ffcs.mcu.RtspEndPointManager" scope="application" />
<fieldset>
    <legend><img src="icons/brick.png"> 添加摄像头</legend>
    <form method="POST" action="controller/addRtspEndPoint">
        <table class="form">
            <tr>
                <td>名称:</td>
                <td><input type="text" style="width:200px;" name="name"></td>
            </tr>
            <tr>
                <td>地址:</td>
                <td><input type="text" style="width:200px;" name="url"></td>
            </tr>
        </table>
        <input class="accept" type="submit" value="创建">
        <input class="cancel" type="submit" onClick="document.location.href='index.jsp';return false;" value="取消">
    </form>
</fieldset>