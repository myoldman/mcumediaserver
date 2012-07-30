<%@page contentType="text/html"%>
<%@page pageEncoding="UTF-8"%>
<%@page import="java.util.Vector"%> 
<%@page import="org.murillo.mcuWeb.ConferenceMngr"%>
<%
    //Get conference manager
    ConferenceMngr confMngr = (ConferenceMngr) getServletContext().getAttribute("confMngr");
%>
<fieldset>
    <legend><img src="icons/brick.png"> 添加媒体服务器</legend>
    <form method="POST" action="controller/addMixer">
        <table class="form">
            <tr>
                <td>名称:</td>
                <td><input type="text" name="name"></td>
            </tr>
            <tr>
                <td>地址:</td>
                <td><input type="text" name="url"></td>
            </tr>
            <tr>
                <td>媒体IP:</td>
                <td><input type="text" name="ip"></td>
            </tr>
	    <tr>
                <td>公网IP:</td>
                <td><input type="text" name="publicIp"></td>
            </tr>
	    <tr>
                <td>私网地址:</td>
                <td><input type="text" name="localNet"></td>
            </tr>
        </table>
        <input class="accept" type="submit" value="创建">
        <input class="cancel" type="submit" onClick="document.location.href='index.jsp';return false;" value="取消">
    </form>
</fieldset>