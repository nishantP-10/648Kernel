package edu.cmu.ziyuans.reservationui;


	import java.io.BufferedReader;
	import java.io.BufferedWriter;
	import java.io.File;
	import java.io.FileInputStream;
	import java.io.FileOutputStream;
	import java.io.IOException;
	import java.io.InputStreamReader;
	import java.io.OutputStreamWriter;
	import java.util.*;

	import android.R.color;
	import android.os.Bundle;
	import android.app.Activity;
	import android.graphics.Color;
	import android.util.Log;
	import android.view.Menu;
	import android.widget.LinearLayout;
	import android.widget.RelativeLayout;
	import android.widget.Spinner;
	import android.widget.TableLayout;
	import android.widget.TableRow;
	import android.widget.TextView;
	public class  Energy extends Activity{
		private TableLayout table;
	    @Override
	    protected void onCreate(Bundle savedInstanceState) {
	        super.onCreate(savedInstanceState);
	        setContentView(R.layout.scroll);
	        enabled(true);
	        table = (TableLayout)findViewById(R.id.table);
	        Timer timer = new Timer();
	        timer.schedule(new TimerTask() {
	        	public void run(){
	        		runOnUiThread(new Runnable(){
	        			public void run() {
	        				fresh();
	        			}
	        		});
	        	}
	        }, 1000, 1000);
	    }
	    private TextView createTextView(String text){
	    	TextView view = new TextView(this);
	    	view.setText(text);
	    	view.setWidth(180);
	    	view.setHeight(30);
	    	return view;
	    }
		public void fresh() {
			String freq = readFromFile("/sys/rtes/freq");
			String power = readFromFile("/sys/rtes/power");
			//String energy = readFromFile("/sys/rtes/energy");
			File dir = new File("/sys/rtes/tasks/");
			table.removeAllViews();
			TableRow first = new TableRow(this);
			first.addView(createTextView("TID"));
			first.addView(createTextView("FREQ(MHZ)"));
			first.addView(createTextView("POWER(mW)"));
			first.addView(createTextView("ENERGY(mJ)"));
			table.addView(first);
			for (File file: dir.listFiles()) {
				String id = file.getName();
				String energy_t = readFromFile("/sys/rtes/tasks/" + id + "/energy");
				TableRow row = new TableRow(this);
				row.addView(createTextView(id));
				row.addView(createTextView(freq));
				row.addView(createTextView(power));
				row.addView(createTextView(energy_t));
				this.table.addView(row);
			}
		}
		private void enabled(boolean enabled) {
			String path = "/sys/rtes/config/energy";
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
		
		private String readFromFile(String path) {
		    String ret = "";
		    File file = new File(path);
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
		
		private void println(String info) {
			Log.i("INFO", info + "\n");
		}
	    @Override
	    public boolean onCreateOptionsMenu(Menu menu) {
	        // Inflate the menu; this adds items to the action bar if it is present.
	        getMenuInflater().inflate(R.menu.main, menu);
	        return true;
	    }
	}
