package PYD.Venduino.java;

import com.intersys.gateway.BusinessOperation;

import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttMessage;
import org.eclipse.paho.client.mqttv3.persist.MemoryPersistence;

import java.io.FileOutputStream;
import java.io.PrintWriter;
import java.io.StringReader;

import javax.xml.parsers.DocumentBuilderFactory;
import org.w3c.dom.Document;
import org.w3c.dom.NodeList;
import org.xml.sax.InputSource;

//As messages will be sent to the device only occasionally, we'll open 
//up a connection to the MQTT target every time we're sending a message
public class MqttBO implements BusinessOperation { 	
	public static final String SETTINGS = "MQTTServer,MQTTPort,MQTTClientName,MQTTTopicRoot,LogFile";
	private String mqttServer="localhost";
	private String mqttPort="1883";
	private String mqttClientName="EnsembleBO";
	private String mqttTopicRoot="Venduino/Dev/A/";  //Only Send to Device A

	private PrintWriter logFile = new PrintWriter(System.out, true);
	
	/*
	public static void main(String[] args) {
		try {
			System.out.println("in Main");
			PYD.Venduino.java.MqttBO bo= new PYD.Venduino.java.MqttBO();
			String[] strArr={"-MQTTServer", "localhost","-MQTTPort", "1883", "-MQTTClientName", "EnsBO" ,"-MQTTTopicRoot", "Venduino/Dev/A", "-LogFile", "c:\\temp\\mqtt.log" };
			System.out.println("onInit: "+ bo.onInitBO(strArr));
		}catch(Exception e) {
			e.printStackTrace();	
		}
	}
	*/
	@Override
	public boolean onInitBO(String[] arg0) throws Exception {

		try {
			// Get Business Operation setting
            for (int i = 0; i < arg0.length-1; i++) {
                if (arg0[i] != null && arg0[i].equals("-MQTTServer")) {
                	this.mqttServer = arg0[++i];
                }
                if (arg0[i] != null && arg0[i].equals("-MQTTPort")) {
                	this.mqttPort = arg0[++i];
                }
                if (arg0[i] != null && arg0[i].equals("-MQTTClientName")) {
                	this.mqttClientName = arg0[++i];
                }	  
                if (arg0[i] != null && arg0[i].equals("-MQTTTopicRoot")) {
                	this.mqttTopicRoot = arg0[++i];
                }
                if (arg0[i] !=null & arg0[1].equals("-LogFile")){
    				this.logFile= new PrintWriter(new FileOutputStream(arg0[++i], true));
    			}
            }
    
		} catch (Exception e) {
			
			e.printStackTrace();
			
		}
		return true;
	}

	@Override
	public boolean onMessage(String arg0) throws Exception {

	    int qos = 0;
	    MemoryPersistence persistence = new MemoryPersistence();
	    MqttMessage message=null;	
	    String topic=this.mqttTopicRoot;
	    String content="";
	    
	    try {
	    	this.logFile.println("Received Message:"+arg0);
	    	// Parse xml
	    	InputSource is = new InputSource(new StringReader(arg0));
	    	DocumentBuilderFactory fact=DocumentBuilderFactory.newInstance();
	    	fact.setNamespaceAware(true);
	    	Document doc = fact.newDocumentBuilder().parse(is);   
	    	doc.getDocumentElement().normalize();
	    	
	    	this.logFile.println("Parsed XML");
	    	
	    	//get type from xml
	    	NodeList nl=doc.getElementsByTagNameNS("http://venduino.org/message.xsd","type");
	    	if (nl.getLength()>0) {
	    				
	    			topic=topic+nl.item(0).getTextContent();
	    	}
	    	this.logFile.println("Full topic:"+topic);
	    	
	    	// Get content from xml
	    	nl=doc.getElementsByTagNameNS("http://venduino.org/message.xsd","content");
	    	if (nl.getLength()>0) {
	    		content=nl.item(0).getTextContent();
	    	}
	    	this.logFile.println("Full content:"+content);    	
	        MqttClient client =new MqttClient("tcp://"+this.mqttServer+":"+this.mqttPort, this.mqttClientName,persistence);  
	        MqttConnectOptions connOpts = new MqttConnectOptions();
	        connOpts.setCleanSession(true);
	        client.connect(connOpts);

	        message = new MqttMessage(content.getBytes());
	        message.setQos(qos);
	        client.publish(topic, message);
	        client.disconnect();
	    } catch (Exception e) {
	    	e.printStackTrace(this.logFile);
	    	throw e;
	    	
	    }
		return true;
	}

	@Override
	public boolean onTearDownBO() throws Exception {
		// TODO Auto-generated method stub
		return true;
	}

}
