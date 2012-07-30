/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package com.ffcs.mcu;

import com.ffcs.mcu.pojo.RtspEndPoint;
import com.ffcs.mcu.pojo.EndPoint.State;
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
import javax.sql.DataSource;

/**
 *
 * @author liuhong
 */
public enum RtspEndPointManager {
    
    INSTANCE;
    private final Map<Integer, RtspEndPoint> rtspEndPoints = new HashMap<Integer, RtspEndPoint>();
    private Context initCtx;
    private DataSource ds;
    private final Logger logger = Logger.getLogger(RtspEndPointManager.class.getName());
    

    RtspEndPointManager() {
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
                ResultSet rs = stmt.executeQuery("select * from rtsp_endpoint");
                synchronized (rtspEndPoints) {
                    while (rs.next()) {
                        RtspEndPoint rtspEndPoint = new RtspEndPoint();
                        rtspEndPoint.setId(rs.getInt(1));
                        rtspEndPoint.setName(rs.getString(2));
                        rtspEndPoint.setRtspUrl(rs.getString(3));
                        rtspEndPoint.setState(RtspEndPoint.State.IDLE);
                        rtspEndPoints.put(rtspEndPoint.getId(), rtspEndPoint);
                        logger.log(Level.INFO, "rtsp endpoint name is {0}", rtspEndPoint.getName());
                    }
                }
                rs.close();
                stmt.close();
                conn.close();
            }
        } catch (SQLException ex) {
            Logger.getLogger(RtspEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
        } catch (NamingException ex) {
            Logger.getLogger(RtspEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
        }
    }

    /**
     * @return the rtspEndPoints
     */
    public Map<Integer, RtspEndPoint> getRtspEndPoints() {
        return rtspEndPoints;
    }

    public RtspEndPoint getRtspEndPoint(String uid) {
        int id = Integer.valueOf(uid);
        return rtspEndPoints.get(id);
    }
    
    public void setRtspEndPointState(String uid, State state) {
        int id = Integer.valueOf(uid);
        synchronized (rtspEndPoints) {
            RtspEndPoint endPoint = rtspEndPoints.get(id);
            if(endPoint != null) {
                endPoint.setState(state);
            }
        }
    }
        
    public void removeRtspEndPoint(String uid) {
        int id = Integer.valueOf(uid);
        synchronized (rtspEndPoints) {
            try {
                //Remove mixer
                rtspEndPoints.remove(id);
                Connection conn = null;
                PreparedStatement presta = null;
                conn = ds.getConnection();
                presta = conn.prepareStatement("delete from rtsp_endpoint where id = ?");
                presta.setInt(1, id);
                int flag = presta.executeUpdate();
                if (flag != 0) {
                    logger.log(Level.INFO, "Delete rtsp endpoint {0} success", uid);
                }
                presta.close();
                conn.close();
            } catch (SQLException ex) {
                Logger.getLogger(RtspEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
    }

    public void addRtspEndPoint(String name, String url) {
        try {
            //Create RtspEndpoint
            RtspEndPoint rtspEndPoint = new RtspEndPoint();
            rtspEndPoint.setName(name);
            rtspEndPoint.setRtspUrl(url);
            rtspEndPoint.setState(RtspEndPoint.State.IDLE);
            Connection conn = null;
            PreparedStatement presta = null;
            conn = ds.getConnection();
            presta = conn.prepareStatement("insert into rtsp_endpoint (name,url) values(?,?)", Statement.RETURN_GENERATED_KEYS);
            presta.setString(1, name);
            presta.setString(2, url);
            int flag = presta.executeUpdate();
            if (flag != 0) {
                logger.log(Level.INFO, "Add rtsp endpoint {0} success", name);
                ResultSet rs = presta.getGeneratedKeys(); //获取       
                rs.next();         
                int id = rs.getInt(1);
                rtspEndPoint.setId(id);
            }
            presta.close();
            conn.close();
            synchronized (rtspEndPoints) {
                rtspEndPoints.put(rtspEndPoint.getId(), rtspEndPoint);
            }
            
        } catch (SQLException ex) {
            Logger.getLogger(RtspEndPointManager.class.getName()).log(Level.SEVERE, null, ex);
        }
    }
}
