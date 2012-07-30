<%@page contentType="text/html"%>
<%@page pageEncoding="UTF-8"%>
<%@page import="java.util.Vector"%> 
<jsp:useBean id="sipEndPointManager" class="com.ffcs.mcu.SipEndPointManager" scope="application" />
<fieldset>
    <legend><img src="icons/brick.png"> 添加客户端</legend>
    <form method="POST" action="controller/addSipEndPoint">
        <table class="form">
            <tr>
                <td>名称:</td>
                <td><input type="text" style="width:200px;" name="name"></td>
            </tr>
            <tr>
                <td>号码:</td>
                <td><input type="text" style="width:200px;" name="number"></td>
            </tr>
        </table>
        <input class="accept" type="submit" value="创建">
        <input class="cancel" type="submit" onClick="document.location.href='index.jsp';return false;" value="取消">
    </form>
</fieldset>