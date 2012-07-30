/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ffcs.mcu;

import com.ffcs.mcu.pojo.EndPoint.State;
import com.ffcs.mcu.pojo.SipEndPoint;
import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.HashMap;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.naming.Context;
import javax.naming.InitialContext;
import javax.naming.NamingException;
import javax.servlet.sip.SipURI;
import javax.sql.DataSource;

/**
 *
 * @author liuhong
 */
public enum SipEndPointManager {

    INSTANCE;
    private final Map<Integer, SipEndPoint> sipEndPoints = new HashMap<Integer, SipEndPoint>();
    private Context initCtx;
    private DataSource ds;
    private final Logger logger = Logger.getLogger(SipEndPointManager.class.getName());

    SipEndPointManager() {
        try {
            initCtx = new InitialContext();
            ds = (DataSource) initCtx.lookup("jdbc/MySQlPool");
            if (ds != null) {
                Connection conn = null;
                Statement stmt = null;
                logger.log(Level.INFO, "mysql datasource get success.");
                conn = ds.getConnection();
                stmt = conn.createStatement();
                logger.log(Level.INFO, "mysql success connected.");
                ResultSet rs = stmt.executeQuery("select * from sip_endpoint order by number asc");
                synchronized (sipEndPoints) {
                    while (rs.next()) {
                        SipEndPoint sipEndPoint = new SipEndPoint();
                        sipEndPoint.setId(rs.getInt(1));
                        sipEndPoint.setName(rs.getString(2));
                        sipEndPoint.setNumber(rs.getString(3));
                        sipEndPoint.setState(SipEndPoint.State.NOTREGISTER);
                        sipEndPoints.put(sipEndPoint.getId(), sipEndPoint);
                        logger.log(Level.INFO, "sip endpoint name is {0}", sipEndPoint.getName());
                    }
                }
                rs.close();
                stmt.close();
                conn.close();
            }
        } catch (SQLException ex) {
            Logger.getLogger(SipEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
        } catch (NamingException ex) {
            Logger.getLogger(SipEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    /**
     * @return the sipEndPoints
     */
    public Map<Integer, SipEndPoint> getSipEndPoints() {
        return sipEndPoints;
    }

    public SipEndPoint getSipEndPoint(String uid) {
        int id = Integer.valueOf(uid);
        return sipEndPoints.get(id);
    }
    
    public String getSipEndPointNameByNumber(String number){
        synchronized (sipEndPoints) {
            for (SipEndPoint sipEndPoint : sipEndPoints.values()) {
                if(sipEndPoint.getNumber().equalsIgnoreCase(number))
                    return sipEndPoint.getName();
            }
        }
        return null;
    }
   
    public SipURI getSipEndPointUriByNumber(String number){
        synchronized (sipEndPoints) {
            for (SipEndPoint sipEndPoint : sipEndPoints.values()) {
                if(sipEndPoint.getNumber().equalsIgnoreCase(number))
                    return sipEndPoint.getSipUri();
            }
        }
        return null;
    }
    
    public String existNumber(String number){
        synchronized (sipEndPoints) {
            for (SipEndPoint sipEndPoint : sipEndPoints.values()) {
                if(sipEndPoint.getNumber().equalsIgnoreCase(number))
                    return String.valueOf(sipEndPoint.getId());
            }
        }
        return null;
    }

    public void setSipEndPointStateByNumber(String number, State state){
         synchronized (sipEndPoints) {
            for (SipEndPoint sipEndPoint : sipEndPoints.values()) {
                if(sipEndPoint.getNumber().equalsIgnoreCase(number)) {
                    sipEndPoint.setState(state);
                    return;
                }
            }
        }
    }
    
    public void setSipEndPointState(String uid, State state) {
        int id = Integer.valueOf(uid);
        synchronized (sipEndPoints) {
            SipEndPoint endPoint = sipEndPoints.get(id);
            if (endPoint != null) {
                endPoint.setState(state);
            }
        }
    }

    public void removeSipEndPoint(String uid) {
        int id = Integer.valueOf(uid);
        synchronized (sipEndPoints) {
            try {
                //Remove mixer
                sipEndPoints.remove(id);
                Connection conn = null;
                PreparedStatement presta = null;
                conn = ds.getConnection();
                presta = conn.prepareStatement("delete from sip_endpoint where id = ?");
                presta.setInt(1, id);
                int flag = presta.executeUpdate();
                if (flag != 0) {
                    logger.log(Level.INFO, "Delete sip endpoint {0} success", uid);
                }
                presta.close();
                conn.close();
            } catch (SQLException ex) {
                Logger.getLogger(SipEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    public void addSipEndPoint(String name, String number) {
        try {
            //Create RtspEndpoint
            SipEndPoint sipEndPoint = new SipEndPoint();
            sipEndPoint.setName(name);
            sipEndPoint.setNumber(number);
            sipEndPoint.setState(SipEndPoint.State.NOTREGISTER);
            Connection conn = null;
            PreparedStatement presta = null;
            conn = ds.getConnection();
            presta = conn.prepareStatement("insert into sip_endpoint (name,number) values(?,?)", Statement.RETURN_GENERATED_KEYS);
            presta.setString(1, name);
            presta.setString(2, number);
            int flag = presta.executeUpdate();
            if (flag != 0) {
                logger.log(Level.INFO, "Add rtsp endpoint {0} success", name);
                ResultSet rs = presta.getGeneratedKeys(); //获取       
                rs.next();
                int id = rs.getInt(1);
                sipEndPoint.setId(id);
            }
            presta.close();
            conn.close();
            synchronized (sipEndPoints) {
                sipEndPoints.put(sipEndPoint.getId(), sipEndPoint);
            }

        } catch (SQLException ex) {
            Logger.getLogger(SipEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
