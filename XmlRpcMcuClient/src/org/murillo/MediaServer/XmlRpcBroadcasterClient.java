/*
 * XmlRpcMcuClient.java
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
package org.murillo.MediaServer;

import java.net.MalformedURLException;
import java.net.URL;
import org.apache.xmlrpc.XmlRpcException;
import org.apache.xmlrpc.client.XmlRpcClient;
import org.apache.xmlrpc.client.XmlRpcClientConfigImpl;
import java.util.HashMap;

/**
 *
 * @author Sergio Garcia Murillo
 */
public class XmlRpcBroadcasterClient {

    private XmlRpcClient client;
    private XmlRpcClientConfigImpl config;

    /** Creates a new instance of XmlRpcMcuClient */
    public XmlRpcBroadcasterClient(String  url) throws MalformedURLException
    {
        config = new XmlRpcClientConfigImpl();
        config.setServerURL(new URL(url));
        client = new XmlRpcClient();
        client.setConfig(config);
    }

    public Integer CreateBroadcast(String name,String tag,Integer maxTransfer,Integer maxConcurrent) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{name,tag,maxTransfer,maxConcurrent};
        //Execute
        HashMap response = (HashMap) client.execute("CreateBroadcast", request);
        //Get result
        Object[] returnVal = (Object[]) response.get("returnVal");
        //Return
        return (Integer)returnVal[0];
    }

    public void PublishBroadcast(Integer broadcastId,String pin) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{broadcastId,pin};
        //Execute
        HashMap response = (HashMap) client.execute("PublishBroadcast", request);
    }

    public void UnPublishBroadcast(Integer broadcastId) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{broadcastId};
        //Execute
        HashMap response = (HashMap) client.execute("UnPublishBroadcast", request);
    }

    public void AddBroadcastToken(Integer broadcastId,String token) throws XmlRpcException
    {
         //Create request
        Object[] request = new Object[]{broadcastId,token};
        //Execute
        HashMap response = (HashMap) client.execute("AddBroadcastToken", request);
    }

    public void DeleteBroadcast(Integer broadcastId) throws XmlRpcException
    {
        //Create request
        Object[] request = new Object[]{broadcastId};
        //Execute
        HashMap response = (HashMap) client.execute("DeleteBroadcast", request);
    }


}
