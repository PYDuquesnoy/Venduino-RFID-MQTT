package PYD.Venduino.java;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallback;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttMessage;

//---Ensemble
import com.intersys.gateway.BusinessService;
import com.intersys.gateway.Production;

public class MqttBS implements BusinessService, MqttCallback {
	
	public static final String SETTINGS = "MQTTServer,MQTTPort,MQTTClientName,MQTTTopic";
	private String mqttServer="localhost";
	private String mqttPort="1883";
	private String mqttClientName="EnsembleBS";
	private String mqttTopic="Venduino/Ens/#";
	//---
	
	private MqttClient client;
	private Production production;
	
	public boolean onInitBS(Production prod) throws Exception {
		this.production=prod;
		this.production.logMessage("java, OnInitBS",com.intersys.gateway.Production.Severity.TRACE);
		
		//---Copy settings to Properties
		this.mqttServer=prod.getSetting("MQTTServer");
		this.mqttPort=prod.getSetting("MQTTPort");
		this.mqttClientName=prod.getSetting("MQTTClientName");  //Or use //MqttClient.generateClientId());
		this.mqttTopic=prod.getSetting("MQTTTopic");
		
		//----
		// Create the MqttClient connection to the broker
	    client=new MqttClient("tcp://"+this.mqttServer+":"+this.mqttPort, this.mqttClientName); 
	    client.connect();
	    this.production.logMessage("java, connected",com.intersys.gateway.Production.Severity.TRACE);
	    //----
		//Subscribe to Topic
	    String[]myTopics={this.mqttTopic};
	    
	    client.subscribe(myTopics);  //use Default QoS
		this.production.logMessage("java, subscribed to: "+this.mqttTopic,com.intersys.gateway.Production.Severity.TRACE);
		client.setCallback(this);
		return true;
	}

	@Override
	public boolean onTearDownBS() throws Exception {
		this.production.logMessage("java:onTearDownBS",com.intersys.gateway.Production.Severity.TRACE);
		client.disconnect();
		return true;
	}

	///From MqttCallback
	public void connectionLost(Throwable arg0) {
		// TODO Auto-generated method stub
		try {
			this.production.logMessage("java, connectionLost",com.intersys.gateway.Production.Severity.WARN);
		}catch (Exception e)
		{ 
			//Nothing to do, really
		}
		
	}

	///From MqttCallback interface
	public void deliveryComplete(IMqttDeliveryToken token) {
		// TODO Auto-generated method stub
		try {
			this.production.logMessage("java, Delivery Complete",  com.intersys.gateway.Production.Severity.TRACE);
			this.production.logMessage("java, message Delivered, token:"+token.toString(), com.intersys.gateway.Production.Severity.TRACE);
		}catch (Exception e)
		{
			//Nothing to do, really
		}
	}

	///From MqttCallback interface
	///Always create a default XML Message with format
	///
	///<message>
	///  	<type> Topic-of-mqtt-message 	</type>
	///		<content> Content-of-mqtt-message <content>
	///</message>
	///
	public void messageArrived(String topic, MqttMessage message) throws Exception {
		this.production.logMessage("java, messageArrived", com.intersys.gateway.Production.Severity.TRACE);
		this.production.logMessage("java, topic:  "+topic+"; Message:"+message.toString(),com.intersys.gateway.Production.Severity.TRACE);
		String xmlMessage="<message><type>"+topic+"</type><content>"+message.toString()+"</content></message>";
		
		this.production.logMessage("java, xmlMessage:"+xmlMessage,com.intersys.gateway.Production.Severity.TRACE);
		this.production.sendRequest(xmlMessage);
	}

}
