package tinyboy.views;

import java.awt.BasicStroke;
import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.Rectangle;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;

import javax.swing.JPanel;

import javr.core.AvrPeripheral;
import javrsim.peripherals.JPeripheral;
import tinyboy.core.ControlPad;
import tinyboy.core.TinyBoyEmulator;

/**
 * Provides a simple view of the tiny boy which shows the display and provides
 * buttons for inputs.
 *
 * @author David J. Pearce
 *
 */
public class TinyBoyPeripheral extends JPeripheral {
	private final TinyBoyEmulator tinyBoy;

	public TinyBoyPeripheral(TinyBoyEmulator tinyBoy) {
		super("TinyBoy");
		this.tinyBoy = tinyBoy;
		DisplayPanel dc = new DisplayPanel(256, 256);
		ButtonPanel bp  = new ButtonPanel(256,100);
		// Add panels
		add(dc,BorderLayout.CENTER);
		add(bp, BorderLayout.SOUTH);
		// Set button panel to have focus. This is important as, otherwise, they
		// keyboard will not be redirected there.
		bp.requestFocus();
		//
		//
		setBounds(0, 0, 300, 400);
		setPreferredSize(new Dimension(300,400));
		setResizable(false);
		pack();
		setVisible(true);
	}

	@Override
	public AvrPeripheral getPeripheral() {
		// FIXME: need to update TinyBoy to be an AvrPeripheral.
		return null;
	}

	@Override
	public void clock() {
		tinyBoy.clock();
		repaint();
	}

	private class DisplayPanel extends JPanel {
		/**
		 * Construct display canvas for a given width and height (in pixels). This
		 * determines the resolution at which the display is drawn.
		 *
		 * @param width
		 * @param height
		 */
		public DisplayPanel(int width, int height) {
			setBounds(0, 0, width, height);
			setPreferredSize(new Dimension(width+44, height+44));
		}

		@Override
		public void paint(Graphics g) {
			// Draw outline
			g.setColor(Color.DARK_GRAY);
			g.drawRect(20, 20, getWidth()-40, getHeight()-40);
			g.drawRect(19, 19, getWidth()-38, getHeight()-38);
			// Draw Display
			drawDisplay(g.create(22, 22, getWidth()-44, getHeight()-44));
		}

		/**
		 * Draw the TinyBoy display.
		 *
		 * @param g
		 * @param pw
		 * @param ph
		 */
		private void drawDisplay(Graphics g) {
			int pw = getWidth() / tinyBoy.getDisplayWidth();
			int ph = getHeight() / tinyBoy.getDisplayHeight();
			//
			for (int y = 0; y != tinyBoy.getDisplayHeight(); ++y) {
				for (int x = 0; x != tinyBoy.getDisplayWidth(); ++x) {
					boolean pixel = tinyBoy.isPixelSet(x, y);
					if (pixel) {
						g.setColor(Color.BLACK);
					} else {
						g.setColor(Color.WHITE);
					}
					g.fillRect(x * pw, y * ph, pw, ph);
				}
			}
		}
	}

	private class ButtonPanel extends JPanel implements MouseListener, KeyListener {
		private final Rectangle[] buttons = new Rectangle[4];
		/**
		 * Construct display canvas for a given width and height (in pixels). This
		 * determines the resolution at which the display is drawn.
		 *
		 * @param width
		 * @param height
		 */
		public ButtonPanel(int width, int height) {
			super();
			setBounds(0, 0, width, height);
			setPreferredSize(new Dimension(width,height));
			// Calculate the sizes of buttons
			buttons[ControlPad.Button.UP.ordinal()] = getRectangle(2,0);
			buttons[ControlPad.Button.DOWN.ordinal()] = getRectangle(2,2);
			buttons[ControlPad.Button.LEFT.ordinal()] = getRectangle(1,1);
			buttons[ControlPad.Button.RIGHT.ordinal()] = getRectangle(3,1);
			// Add listener for mouse clicks
			addMouseListener(this);
			addKeyListener(this);
			//
			this.setFocusable(true);
		}

		@Override
		public void paint(Graphics g) {
			Rectangle r = g.getClipBounds();
			g = g.create(r.x,r.y,r.width,r.height);
			// Draw left button
			drawBlackButton(g, buttons[ControlPad.Button.LEFT.ordinal()], tinyBoy.getButtonState(ControlPad.Button.LEFT));
			// Draw up button
			drawBlackButton(g, buttons[ControlPad.Button.UP.ordinal()], tinyBoy.getButtonState(ControlPad.Button.UP));
			// Draw right button
			drawBlackButton(g, buttons[ControlPad.Button.RIGHT.ordinal()], tinyBoy.getButtonState(ControlPad.Button.RIGHT));
			// Draw down button
			drawBlackButton(g, buttons[ControlPad.Button.DOWN.ordinal()], tinyBoy.getButtonState(ControlPad.Button.DOWN));
		}

		private void drawBlackButton(Graphics g, Rectangle rect, boolean state) {
			if (state) {
				g.setColor(Color.DARK_GRAY);
			} else {
				g.setColor(Color.LIGHT_GRAY);

			}
			g.fillRect(rect.x,rect.y+1,rect.width,rect.height);
		    Graphics2D g2 = (Graphics2D) g;
		    g2.setStroke(new BasicStroke(2));
			g.setColor(Color.DARK_GRAY);
			g.drawRect(rect.x,rect.y+1,rect.width,rect.height);
		}

		@Override
		public void mouseClicked(MouseEvent e) {

		}

		@Override
		public void mousePressed(MouseEvent e) {
			ControlPad.Button button = determineButton(e.getX(),e.getY());
			if(button != null) {
				tinyBoy.setButtonState(button, true);
			}
		}

		@Override
		public void mouseReleased(MouseEvent e) {
			ControlPad.Button button = determineButton(e.getX(),e.getY());
			if(button != null) {
				tinyBoy.setButtonState(button, false);
			}
		}

		@Override
		public void mouseEntered(MouseEvent e) {

		}

		@Override
		public void mouseExited(MouseEvent e) {

		}

		@Override
		public void keyTyped(KeyEvent e) {
			// TODO Auto-generated method stub

		}

		@Override
		public void keyPressed(KeyEvent e) {
			ControlPad.Button b = determineButton(e);
			if(b != null) {
				tinyBoy.setButtonState(b, true);
			}

		}

		@Override
		public void keyReleased(KeyEvent e) {
			ControlPad.Button b = determineButton(e);
			if(b != null) {
				tinyBoy.setButtonState(b, false);
			}
		}

		private ControlPad.Button determineButton(int x, int y) {
			for(int i=0;i!=buttons.length;++i) {
				if(buttons[i].contains(x,y)) {
					return ControlPad.Button.values()[i];
				}
			}
			// No hit
			return null;
		}

		private Rectangle getRectangle(int rx, int ry) {
			int rw = getWidth() / 10;
			int rh = getHeight() / 4;
			return new Rectangle(rw * rx, rh * ry, rw, rh);
		}

		private ControlPad.Button determineButton(KeyEvent e) {
			switch (e.getKeyCode()) {
			case KeyEvent.VK_UP:
				return ControlPad.Button.UP;
			case KeyEvent.VK_DOWN:
				return ControlPad.Button.DOWN;
			case KeyEvent.VK_LEFT:
				return ControlPad.Button.LEFT;
			case KeyEvent.VK_RIGHT:
				return ControlPad.Button.RIGHT;
			}
			return null;
		}
	}
}
