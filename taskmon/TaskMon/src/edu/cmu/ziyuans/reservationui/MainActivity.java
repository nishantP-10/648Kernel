package edu.cmu.ziyuans.reservationui;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.text.DecimalFormat;
import java.util.*;

import com.androidplot.ui.AnchorPosition;
import com.androidplot.ui.XLayoutStyle;
import com.androidplot.ui.YLayoutStyle;
import com.androidplot.xy.BoundaryMode;
import com.androidplot.xy.LineAndPointFormatter;
import com.androidplot.xy.PointLabelFormatter;
import com.androidplot.xy.SimpleXYSeries;
import com.androidplot.xy.XYPlot;
import com.androidplot.xy.XYSeries;

import android.os.Bundle;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Intent;
import android.graphics.Color;
import android.util.Log;
import android.view.Menu;
import android.view.MotionEvent;
import android.view.View;
import android.view.View.OnTouchListener;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Spinner;
import android.widget.TextView;

public class MainActivity extends Activity {
	int count = 0;
	
	static { System.loadLibrary("my_native_api");}
    static private HashMap<Integer, LinkedList<Number>> threadMap = new HashMap<Integer, LinkedList<Number>>();
    static private HashMap<Integer, Long> threadMapTime = new HashMap<Integer, Long>();
    private boolean monitoring = false;
	private long beginTimeStamp = 0;
	
	public native String threadList();
	public native String reserveThreadList();
	public native int setReserve(int threadId, int budget, int period, int cpuid);
	public native int cancelReserve(int threadId);
	
	private Spinner spinner1;
	private Spinner spinner2;
	private Button button_setting;
	private Button button_cancel;
	private Button button_monitor;
	private Button button_energy;
	private EditText threadIdText;
	private EditText budgetText;
	private EditText periodText;
	private EditText cpuidText;
	private XYPlot plot;
    private Thread myThread;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //threadUtiMap = new HashMap<Integer, LinkedList<Number>>();
        spinner1 = (Spinner)findViewById(R.id.threadIDList);
        spinner2 = (Spinner)findViewById(R.id.reservedThreadID);
        loadSpinner(spinner1, null);
        loadSpinner(spinner2, null);
        button_setting = (Button) findViewById(R.id.setting);
        button_cancel = (Button) findViewById(R.id.cancel);
        button_monitor = (Button) findViewById(R.id.monitor);
        button_energy = (Button) findViewById(R.id.energy);
        threadIdText = (EditText)findViewById(R.id.threadID);
    	budgetText = (EditText)findViewById(R.id.budgetC);
    	periodText = (EditText)findViewById(R.id.periodT);
    	cpuidText = (EditText)findViewById(R.id.cpuID);
        spinner1.setOnTouchListener(new MySpinnerTouchListener1());
        spinner2.setOnTouchListener(new MySpinnerTouchListener2());
        plot = init_plot();
        listenButtionSet();
        listenButtionCan();
        listenButtionMon();
        listenButtionEng();
    }
    
	private void settingReservation() {
		String threadIdList = spinner1.getSelectedItem().toString();
		String threadId = threadIdText.getText().toString();
		String budget = budgetText.getText().toString();
		String period = periodText.getText().toString();
		String cpuId = cpuidText.getText().toString();
		if (threadId.equals("")) {
			threadId = threadIdList;
		}
		println("Thread id : " + threadId + 
				"\nBudget : " + budget + 
				"\nPeriod : " +period + 
				"\nCpuId : " + cpuId);
		int thread_id;
		int budget_ms;
		int period_ms;
		int cpu_id;
		loadSpinner(spinner1, null);
		try {
			//parse all input
			thread_id = Integer.parseInt(threadId);
			budget_ms = Integer.parseInt(budget);
			period_ms = Integer.parseInt(period);
			cpu_id = Integer.parseInt(cpuId);
			if (cpu_id >= 4 || cpu_id < 0) {
				inputErrorAlert("CPU Id should be in the range between 0 and 3");
				return;
			} else if (budget_ms <= 0) {
				inputErrorAlert("Buddge C should be greater than zero");
				return;
			} else if (period_ms <= 0) {
				inputErrorAlert("Period should be greater than zero");
				return;
			} else if (budget_ms >= period_ms) {
				inputErrorAlert("Buddge C should be less than Period");
				return;
			}
			//TODO with thread_id, budget_ms, period_ms, cpu_id;
			//Call native function
			if (threadMapTime.containsKey(thread_id) && monitoring) {
				putUtilInMap(thread_id, readFromFile(thread_id));
			}
			if (setReserve(thread_id, budget_ms, period_ms, cpu_id) >= 0) {
				threadMapTime.put(thread_id, System.currentTimeMillis());
			}
		} catch (Exception e) {
			inputErrorAlert("All fields must be valid Integer Number.");
			return;
		}
	}
	private void cancelReservation() {
		String threadId = spinner2.getSelectedItem().toString();
		loadSpinner(spinner2, null);
		try {
			//parse input
			int thread_id = Integer.parseInt(threadId);
			//TODO with thread_id
			//Call native function
			if (monitoring) {
				putUtilInMap(thread_id, readFromFile(thread_id));
			}
			if (cancelReserve(thread_id) >= 0) {
				threadMapTime.remove(thread_id);
			}
			
		} catch (Exception e) {
			inputErrorAlert("Thread field must be valid Integer Number.");
			return;
		}
	}
	private void monitorTransaction() {
		//TextView tv = (TextView)findViewById(R.id.averageU);
		ArrayList<String> threads = new ArrayList<String>();
		String temp = reserveThreadList();
		if (temp != null) {
			for (String s: temp.split("\\\n")) {
				if (!s.equals(".") && !s.equals("..")) {
					threads.add(s);
    			}
    		}
		}
		
		if (button_monitor.getText().toString().equals(getString(R.string.monitor_start))) {
			this.monitoring = true;
			this.beginTimeStamp = System.currentTimeMillis();
			button_monitor.setText(R.string.monitor_stop);
			threadMap.clear();
    		for (String threadid: threads) {
    			int id = Integer.parseInt(threadid);
    			//discard all previous data
    			readFromFile(id);
    		}
    		writeToFile(true);
			//tv.setText("Monitoring...");
		} else {
			monitoring = false;
			button_monitor.setText(R.string.monitor_start);
			//tv.setText("Average Utilization: " + count);
//			LinkedList<Number> list = new LinkedList<Number>();
//			threadMap.put(100, list);
//			list.add(2);
//			list.add(0.2);
//			list.add(4);
//			list.add(0.3);
//			list.add(90);
//			list.add(0.8);
			writeToFile(false);
    		for (String threadid: threads) {
    			int id = Integer.parseInt(threadid);
    			String text = readFromFile(id);
    			println("Read from "+ id +" : " + text);
    			putUtilInMap(id, text);
    		}
    		this.beginTimeStamp = 0;
			plotPoint(threadMap);
		}
	}
	private void writeToFile(boolean enabled) {
		String path = "/sys/rtes/taskmon/enabled";
		File file = new File(path);
		try {
			OutputStreamWriter writer = new OutputStreamWriter(new FileOutputStream(file));
	    	BufferedWriter bWriter = new BufferedWriter(writer);
	    	if (enabled) {
	    		bWriter.write("1");
	    	} else {
	    		bWriter.write("0");
	    	}
	    	bWriter.close();
		} catch (IOException e) {
    		e.printStackTrace();
    	}
	}
	private String readFromFile(int id) {
	    String ret = "";
	    //File file = new File("/data/lab2/files/100);
	    File file = new File("/sys/rtes/taskmon/util/"+ id);
	    try {
	    	InputStreamReader reader = new InputStreamReader(new FileInputStream(file));
	    	BufferedReader bReader = new BufferedReader(reader);
	    	String line = "";
	    	while ((line = bReader.readLine()) != null) {
	    		ret += line + "\n";
	    		println(line);
	    	}
	    	bReader.close();
	    } catch (IOException e){
	    	e.printStackTrace();
	    }
	    return ret;
	}
	private void putUtilInMap(int threadid, String text) {
		if (!threadMapTime.containsKey(threadid)) {
			println("Thread: " + threadid + " is not reversed in this manager.");
			return;
		}
		if (!threadMap.containsKey(threadid)) {
			LinkedList<Number> list = new LinkedList<Number>();
			threadMap.put(threadid, list);
		}
		
		for(String line: text.split("\\\n")) {
			println("line: " + line);
			String[] pair = line.split("\\\t");
			try {
				double offset = Double.parseDouble(pair[0]);
				double util = Double.parseDouble(pair[1]);
				
				double time = threadMapTime.get(threadid) - this.beginTimeStamp + offset;
				if (time >= 0) {
					threadMap.get(threadid).add(time);
					threadMap.get(threadid).add(util);
				}
			} catch (Exception e) {
				println("discard invalid line : " + line);
			}
		}
	}
	private void println(String info) {
		Log.i("INFO", info + "\n");
	}
	private void inputErrorAlert(String reason) {
		new AlertDialog.Builder(this)
		.setTitle("Error Input")
		.setMessage(reason)
		.setIcon(android.R.drawable.ic_dialog_alert)
		.show();
	}
	private void popupMeg(String title, String text) {
		new AlertDialog.Builder(this)
		.setTitle(title)
		.setMessage(text)
		.setIcon(android.R.drawable.ic_dialog_alert)
		.show();
	}
	private void loadSpinner(Spinner spinner, ArrayList<String> item) {
		ArrayList<String> localItems;
		if (item == null) {
			localItems = new ArrayList<String>();
		} else {
			Collections.sort(item, new Comparator<String>() {
				@Override
				public int compare(String s1, String s2) {
					return Integer.parseInt(s1) - Integer.parseInt(s2);
				}
			});
			localItems = new ArrayList<String>(item);
		}
		localItems.add(0, "#");
		localItems.add("Select ThreadID");
		ArrayAdapter<String> adapter = new ArrayAdapter<String>(this
				, android.R.layout.simple_spinner_dropdown_item,localItems) {
			@Override
            public int getCount() {
                return super.getCount()-1;
            }
		};
		spinner.setAdapter(adapter);
		spinner.setSelection(spinner.getCount());
	}
	private void listenButtionSet() {
		button_setting.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				println("Set Button clicked");
				settingReservation();
			}
		});
	}
    private void listenButtionCan() {
    	button_cancel.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				println("Cancel Button clicked");
				cancelReservation();
			}
		});
	}
    private void listenButtionMon() {
    	button_monitor.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				println("Monitor Button clicked");
				monitorTransaction();
			}
		});
	}
    private void listenButtionEng() {
    	button_energy.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View v) {
				println("Energy Button clicked");
				startAnotherActivity();
			}
		});
    }
    private void startAnotherActivity(){
    	startActivity(new Intent(this, Energy.class));
    }
    class MySpinnerTouchListener1 implements OnTouchListener {
    	@Override
    	public boolean onTouch(View v, MotionEvent event) {
    		ArrayList<String> items = new ArrayList<String>();
    		//TODO Update the list of threads id, put all thread id into the items list
    		/*** Modify Code between comments ***/
    		//Call native function threadList() to get list split by \n
    		String temp = threadList();
    		if (temp != null) {
    			for (String s: temp.split("\\\n")) {
        			items.add(s);
        		}
    		}
    		/*** End ***/
    		loadSpinner(spinner1, items);
    		return false;
    	}
    }
    class MySpinnerTouchListener2 implements OnTouchListener {
    	@Override
    	public boolean onTouch(View v, MotionEvent event) {
    		ArrayList<String> items = new ArrayList<String>();
    		//TODO Update the list of threads id, put all thread id into the items list
    		/*** Modify Code between comments ***/
    		//Call native function threadList() to get list split by \n
    		String temp = reserveThreadList();
    		if (temp != null) {
    			for (String s: temp.split("\\\n")) {
    				if (!s.equals(".") && !s.equals("..")) {
    					items.add(s);
        			}
        		}
    		}
    		/*** End ***/
    		loadSpinner(spinner2, items);
    		return false;
    	}
    }
    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.main, menu);
        return true;
    }
	/**************** Plot********************/
    private XYPlot init_plot() {
    	XYPlot plot = (XYPlot) findViewById(R.id.xyPlot);
	    plot.setRangeBoundaries(0, 1, BoundaryMode.FIXED);
	    plot.setDomainBoundaries(0, BoundaryMode.FIXED, 100, BoundaryMode.AUTO);
	    //plot.setTicksPerRangeLabel(2);
	   /* plot.getLegendWidget().position( 
	    		20, XLayoutStyle.ABSOLUTE_FROM_RIGHT,
	    		25, YLayoutStyle.ABSOLUTE_FROM_TOP,
	    		AnchorPosition.RIGHT_TOP);
	    */
	   
	    plot.getGraphWidget().setDomainLabelOrientation(-45);
	    return plot;
    }
	private void plotPoint(Map<Integer, LinkedList<Number>> map) {
		plot.clear();
		println("plot : \n");
		String shownAverage = "Thread#\t:\tAverage";
		for (Map.Entry<Integer, LinkedList<Number>> task: map.entrySet()) {
			LinkedList<Number> xySeriesNumbers = task.getValue();
			double average = 0;
			int i = 0;
			Integer threadId = task.getKey();
			println("[Thread id]:" + threadId);
			
			for (Number n : xySeriesNumbers) {
				if (i % 2 == 1) {
					average += n.doubleValue();
				}
				i++;
			}
			if (i != 0) {
				average = average / (i / 2);
			}
			shownAverage += "\n" + threadId + "\t:\t" + new DecimalFormat("#.###").format(average);
			XYSeries line = new SimpleXYSeries(xySeriesNumbers, SimpleXYSeries.ArrayFormat.XY_VALS_INTERLEAVED, "#" + threadId);                            
			Random rnd = new Random();
			int color = Color.argb(255, rnd.nextInt(), rnd.nextInt(), rnd.nextInt());
			//LineAndPointFormatter format = new LineAndPointFormatter(color, color, null, new PointLabelFormatter(Color.WHITE));
			LineAndPointFormatter format = new LineAndPointFormatter(color, color, null, null);

			plot.addSeries(line, format);
		}
		popupMeg("List of Average Util of each thread", shownAverage);
        plot.redraw();
	}
}
