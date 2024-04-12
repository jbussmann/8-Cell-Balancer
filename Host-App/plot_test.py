import matplotlib.pyplot as pyplot

# create and modify figure, create row of two axes
figure = pyplot.figure()
# figure.canvas.set_window_title('window title')
figure.set_size_inches(8, 4)
figure.suptitle('figure title')
figure.set_tight_layout(True)
axes_left, axes_right = figure.subplots(1, 2)

# plot data, modify returned line objects, modify axes
axes_left.plot([0, 2, 1, 6, 0], label='left 1')
line_left2, = axes_left.plot([7, 8, 3, 0, 7])
line_left2.set_label('left 2')
axes_left.set_xlabel('left x-label')
axes_left.set_ylabel('left y-label')
axes_left.set_title('left axes title')
axes_left.legend(loc='center left')

# plot data, retrieve y axis from axes, modify y axis
axes_right.plot([1, 4, 5, 5, 6], label='right')
axes_right.set_xlabel('right x-label')
axes_right.set_ylabel('right y-label')
axes_right.set_title('right axes title')
axes_right.legend(loc='center right')
yaxis_right = axes_right.get_yaxis()
yaxis_right.set_ticks_position('right')
yaxis_right.set_label_position('right')

# make figure visible by plotting it
pyplot.show()